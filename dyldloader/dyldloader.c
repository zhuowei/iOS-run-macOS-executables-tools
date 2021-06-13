#include <errno.h>
#include <fcntl.h>
#include <mach-o/fat.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

void set_all_images_offset_maybe(struct segment_command_64* seg_cmd,
                                 size_t* all_images_offset) {
  struct section_64* sections = (void*)seg_cmd + sizeof(*seg_cmd);
  for (unsigned int index = 0; index < seg_cmd->nsects; index++) {
    struct section_64* section = &sections[index];
    if (!strncmp(section->sectname, "__all_image_info",
                 sizeof(section->sectname))) {
      *all_images_offset = section->addr;
    }
  }
}

size_t get_address_space_size(void* executable_region,
                              size_t* all_images_offset, size_t* sigcmd_dataoff,
                              size_t* sigcmd_datasize) {
  struct mach_header_64* mh = executable_region;
  struct load_command* cmd = executable_region + sizeof(struct mach_header_64);

  *all_images_offset = 0;

  uint64_t min_addr = ~0;
  uint64_t max_addr = 0;
  for (unsigned int index = 0; index < mh->ncmds; index++) {
    switch (cmd->cmd) {
      case LC_SEGMENT_64: {
        struct segment_command_64* seg_cmd = (struct segment_command_64*)cmd;
        min_addr = MIN(min_addr, seg_cmd->vmaddr);
        max_addr = MAX(max_addr, seg_cmd->vmaddr + seg_cmd->vmsize);
        if (!strncmp(seg_cmd->segname, "__DATA", sizeof(seg_cmd->segname))) {
          set_all_images_offset_maybe(seg_cmd, all_images_offset);
        }
        break;
      }
      case LC_CODE_SIGNATURE: {
        struct linkedit_data_command* signature_cmd =
            (struct linkedit_data_command*)cmd;
        *sigcmd_dataoff = signature_cmd->dataoff;
        *sigcmd_datasize = signature_cmd->datasize;
      }
    }
    cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
  }
  return max_addr - min_addr;
}

#ifdef __arm64__
static const cpu_type_t kPreferredCpuType = CPU_TYPE_ARM64;
#else
static const cpu_type_t kPreferredCpuType = CPU_TYPE_X86_64;
#endif

off_t get_fat_offset(void* fat_region) {
  struct fat_header* fat_header = fat_region;
  if (fat_header->magic != FAT_CIGAM) {
    fprintf(stderr, "Not a FAT executable. Assume raw.\n");
    return 0;
  }
  struct fat_arch* fat_arches = fat_region + sizeof(struct fat_header);
  for (unsigned int index = 0; index < ntohl(fat_header->nfat_arch); index++) {
    struct fat_arch* fat_arch = &fat_arches[index];
    if (ntohl(fat_arch->cputype) == kPreferredCpuType) {
      return ntohl(fat_arch->offset);
    }
  }
  fprintf(stderr, "No preferred slice\n");
  return -1;
}

int remap_into_process(task_t target_task, void* executable_region,
                       vm_address_t target_base, int executable_fd,
                       off_t fat_offset) {
  struct mach_header_64* mh = executable_region;
  struct load_command* cmd = executable_region + sizeof(struct mach_header_64);
  kern_return_t err;

  for (unsigned int index = 0; index < mh->ncmds; index++) {
    switch (cmd->cmd) {
      case LC_SEGMENT_64: {
        struct segment_command_64* seg_cmd = (struct segment_command_64*)cmd;
        vm_address_t source_address =
            (vm_address_t)(executable_region + seg_cmd->fileoff);
        bool need_remap = true;
        if (need_remap) {
          void* remap =
              mmap(NULL, seg_cmd->filesize, seg_cmd->initprot, MAP_PRIVATE,
                   executable_fd, fat_offset + seg_cmd->fileoff);
          if (remap == MAP_FAILED) {
            fprintf(stderr, "remap failed: %s", strerror(errno));
            return 1;
          }
          source_address = (vm_address_t)remap;
        }
        vm_address_t target_address = target_base + seg_cmd->vmaddr;
        vm_prot_t cur_protection;
        vm_prot_t max_protection;
        if (seg_cmd->filesize) {
          err = vm_remap(target_task, &target_address, seg_cmd->filesize,
                         /*mask=*/0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
                         mach_task_self(), source_address,
                         /*copy=*/false, &cur_protection, &max_protection,
                         VM_INHERIT_COPY);
          if (err) {
            fprintf(stderr, "Can't map into process: %s\n",
                    mach_error_string(err));
            return 1;
          }
        }
        err = vm_protect(target_task, target_address, seg_cmd->vmsize, false,
                         seg_cmd->initprot);
        if (err) {
          fprintf(stderr, "Can't protect into process: %s\n",
                  mach_error_string(err));
          return 1;
        }
        break;
      }
        // TODO(zhuowei): handle unixthread (we currently assume dylds have the
        // same unixthread)
    }
    cmd = (struct load_command*)((char*)cmd + cmd->cmdsize);
  }
  return 0;
}

int set_entry_point(task_t target_task, vm_address_t entry_point) {
  kern_return_t err;
  thread_act_t* thread_array;
  mach_msg_type_number_t num_threads;
  err = task_threads(target_task, &thread_array, &num_threads);
  if (err) {
    fprintf(stderr, "Failed to get threads: %s\n", mach_error_string(err));
    return 1;
  }
  thread_t main_thread = thread_array[0];
#ifdef __x86_64__
  x86_thread_state64_t thread_state;
  mach_msg_type_number_t thread_state_count = x86_THREAD_STATE64_COUNT;
  err = thread_get_state(main_thread, x86_THREAD_STATE64,
                         (thread_state_t)&thread_state, &thread_state_count);
  if (err) {
    fprintf(stderr, "Failed to get thread state: %s\n", mach_error_string(err));
    return 1;
  }
  thread_state.__rip = entry_point;
  err = thread_set_state(main_thread, x86_THREAD_STATE64,
                         (thread_state_t)&thread_state, thread_state_count);
  if (err) {
    fprintf(stderr, "Failed to set thread state: %s\n", mach_error_string(err));
    return 1;
  }
#elif __arm64__
  arm_thread_state64_t thread_state;
  mach_msg_type_number_t thread_state_count = ARM_THREAD_STATE64_COUNT;
  err = thread_get_state(main_thread, ARM_THREAD_STATE64,
                         (thread_state_t)&thread_state, &thread_state_count);
  if (err) {
    fprintf(stderr, "Failed to get thread state: %s\n", mach_error_string(err));
    return 1;
  }
  thread_state.__pc = entry_point;
  err = thread_set_state(main_thread, ARM_THREAD_STATE64,
                         (thread_state_t)&thread_state, thread_state_count);
  if (err) {
    fprintf(stderr, "Failed to set thread state: %s\n", mach_error_string(err));
    return 1;
  }
#endif
  return 0;
}

vm_address_t get_dyld_target_map_address(task_t target_task,
                                         size_t new_dyld_all_images_offset) {
#if 0
  kern_return_t err;
  // https://opensource.apple.com/source/dyld/dyld-195.6/unit-tests/test-cases/all_image_infos-cache-slide/main.c
  task_dyld_info_data_t task_dyld_info;
  mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
  err = task_info(target_task, TASK_DYLD_INFO, (task_info_t)&task_dyld_info,
                  &count);
  if (err) {
    fprintf(stderr, "Failed to get task info: %s\n", mach_error_string(err));
    return 0;
  }
  vm_address_t old_dyld_all_images_address = task_dyld_info.all_image_info_addr;
  return old_dyld_all_images_address - new_dyld_all_images_offset;
#endif
  return 0x160000000;
}

int map_dyld(int target_pid, const char* dyld_path) {
  task_t target_task;
  kern_return_t err;

  err = task_for_pid(mach_task_self(), target_pid, &target_task);
  if (err) {
    fprintf(stderr, "Failed to get task port: %s\n", mach_error_string(err));
    return 1;
  }
  if (target_task == MACH_PORT_NULL) {
    fprintf(stderr, "Can't get task port for pid %d\n", target_pid);
    return 1;
  }

  int dyld_fd = open(dyld_path, O_RDONLY);
  if (dyld_fd == -1) {
    fprintf(stderr, "Can't open %s: %s", dyld_path, strerror(errno));
    return 1;
  }
  struct stat dyld_stat;
  if (fstat(dyld_fd, &dyld_stat)) {
    fprintf(stderr, "Can't stat %s: %s", dyld_path, strerror(errno));
    return 1;
  }
  void* dyld_map =
      mmap(NULL, dyld_stat.st_size, PROT_READ, MAP_PRIVATE, dyld_fd, 0);
  if (dyld_map == MAP_FAILED) {
    fprintf(stderr, "Can't map %s: %s", dyld_path, strerror(errno));
    return 1;
  }
  off_t fat_offset = get_fat_offset(dyld_map);
  if (fat_offset == -1) {
    return 1;
  }
  void* executable_map = dyld_map + fat_offset;
  if (!executable_map) {
    return 1;
  }
  size_t new_dyld_all_images_offset = 0;
  size_t sigcmd_dataoff = 0;
  size_t sigcmd_datasize = 0;
  size_t address_space_size =
      get_address_space_size(executable_map, &new_dyld_all_images_offset,
                             &sigcmd_dataoff, &sigcmd_datasize);
  if (!new_dyld_all_images_offset) {
    fprintf(stderr, "can't find all images\n");
    return 1;
  }
  // ImageLoaderMachO::loadCodeSignature, Loader::mapImage
  fsignatures_t siginfo;
  siginfo.fs_file_start = fat_offset;  // start of mach-o slice in fat file
  siginfo.fs_blob_start =
      (void*)(sigcmd_dataoff);             // start of CD in mach-o file
  siginfo.fs_blob_size = sigcmd_datasize;  // size of CD
  if (fcntl(dyld_fd, F_ADDFILESIGS_RETURN, &siginfo) == -1) {
    fprintf(stderr, "can't add signatures: %s\n", strerror(errno));
    return 1;
  }
  // TODO(zhuowei): this _only_ works if ASLR is enabled
  // (since we try to align the image infos of the new dyld on top of the old,
  // and that would overwrite the executable if ASLR is off and dyld is right
  // behind the executable) at least detect if we would overwrite an existing
  // mapping...
  vm_address_t target_address =
      get_dyld_target_map_address(target_task, new_dyld_all_images_offset);
  if (!target_address) {
    return 1;
  }
  fprintf(stderr, "mapping dyld at %p\n", (void*)target_address);
  err = vm_allocate(target_task, &target_address, address_space_size,
                    VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE);
  if (err) {
    fprintf(stderr, "Can't allocate into process: %s\n",
            mach_error_string(err));
    return 1;
  }
  if (remap_into_process(target_task, executable_map, target_address, dyld_fd,
                         fat_offset)) {
    return 1;
  }
  // TODO(zhuowei): grab entry point from unixthread
  if (set_entry_point(target_task, target_address + 0x45c0)) {
    return 1;
  }
  return 0;
}

int main(int argc, char** argv) {
  if (argc != 3) {
    fprintf(stderr, "usage: dyldloader <path_to_dyld> <pid>\n");
    return 1;
  }
  int err = map_dyld(atoi(argv[2]), argv[1]);
  if (err) {
    return 1;
  }
  return 0;
}
