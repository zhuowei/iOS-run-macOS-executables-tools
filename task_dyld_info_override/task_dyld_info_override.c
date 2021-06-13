#include <stdio.h>
#include <mach/mach.h>

#define DYLD_INTERPOSE(_replacement, _replacee)                               \
  __attribute__((used)) static struct {                                       \
    const void* replacement;                                                  \
    const void* replacee;                                                     \
  } _interpose_##_replacee __attribute__((section("__DATA,__interpose"))) = { \
      (const void*)(unsigned long)&_replacement, (const void*)(unsigned long)&_replacee}

kern_return_t wdb_task_info(task_name_t target_task, task_flavor_t flavor, task_info_t task_info_out, mach_msg_type_number_t *task_info_outCnt) {
	kern_return_t err = task_info(target_task, flavor, task_info_out, task_info_outCnt);
	if (flavor == TASK_DYLD_INFO && target_task != mach_task_self()) {
		task_dyld_info_data_t* d = (task_dyld_info_data_t*)task_info_out;
		fprintf(stderr, "Overriding task_dyld_info %d: %p original\n", (int)target_task, (void*)d->all_image_info_addr);
		d->all_image_info_addr = 0x160000000ull + 0x6c140ull;
	}
	return err;
}
DYLD_INTERPOSE(wdb_task_info, task_info);
