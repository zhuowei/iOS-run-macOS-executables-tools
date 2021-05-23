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

size_t get_address_space_size(void* executable_region) {
  struct mach_header_64* mh = executable_region;
  struct load_command* cmd = executable_region + sizeof(struct mach_header_64);
  uint64_t min_addr = ~0;
  uint64_t max_addr = 0;
  for (unsigned int index = 0; index < mh->ncmds; index++) {
    switch (cmd->cmd) {
      case LC_SEGMENT_64: {
        struct segment_command_64* seg_cmd = (struct segment_command_64*)cmd;
        min_addr = MIN(min_addr, seg_cmd->vmaddr);
        max_addr = MAX(max_addr, seg_cmd->vmaddr + seg_cmd->vmsize);
        break;
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

void* get_executable_map_from_fat(void* fat_region) {
  struct fat_header* fat_header = fat_region;
  if (fat_header->magic != FAT_CIGAM) {
    fprintf(stderr, "Not a FAT executable\n");
    return NULL;
  }
  struct fat_arch* fat_arches = fat_region + sizeof(struct fat_header);
  for (unsigned int index = 0; index < ntohl(fat_header->nfat_arch); index++) {
    struct fat_arch* fat_arch = &fat_arches[index];
    if (ntohl(fat_arch->cputype) == kPreferredCpuType) {
      return fat_region + ntohl(fat_arch->offset);
    }
  }
  fprintf(stderr, "No preferred slice\n");
  return NULL;
}

int remap_into_process(task_t target_task, void* executable_region,
                       vm_address_t target_base) {
  struct mach_header_64* mh = executable_region;
  struct load_command* cmd = executable_region + sizeof(struct mach_header_64);
  kern_return_t err;

  for (unsigned int index = 0; index < mh->ncmds; index++) {
    switch (cmd->cmd) {
      case LC_SEGMENT_64: {
        struct segment_command_64* seg_cmd = (struct segment_command_64*)cmd;
        vm_address_t target_address = target_base + seg_cmd->vmaddr;
        vm_prot_t cur_protection;
        vm_prot_t max_protection;
        if (seg_cmd->filesize) {
          err = vm_remap(target_task, &target_address, seg_cmd->filesize,
                         /*mask=*/0, VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE,
                         mach_task_self(),
                         (vm_address_t)(executable_region + seg_cmd->fileoff),
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
  void* executable_map = get_executable_map_from_fat(dyld_map);
  if (!executable_map) {
    return 1;
  }
  size_t address_space_size = get_address_space_size(executable_map);
  vm_address_t target_address = 0x400000000ull;
  err = vm_allocate(target_task, &target_address, address_space_size,
                    VM_FLAGS_FIXED | VM_FLAGS_OVERWRITE);
  if (err) {
    fprintf(stderr, "Can't allocate into process: %s\n",
            mach_error_string(err));
    return 1;
  }
  if (remap_into_process(target_task, executable_map, target_address)) {
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
