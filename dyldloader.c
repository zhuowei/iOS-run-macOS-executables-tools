#include <errno.h>
#include <fcntl.h>
#include <mach-o/loader.h>
#include <mach/mach.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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
  vm_address_t target_address;
  vm_prot_t cur_protection;
  vm_prot_t max_protection;
  err = vm_remap(target_task, &target_address, dyld_stat.st_size, /*mask=*/0,
                 /*anywhere=*/true, mach_task_self(), (vm_address_t)dyld_map,
                 /*copy=*/false, &cur_protection, &max_protection,
                 VM_INHERIT_COPY);
  if (err) {
    fprintf(stderr, "Can't map into process: %s\n", mach_error_string(err));
    return 1;
  }
  fprintf(stderr, "%p\n", (void*)target_address);
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
