#include <mach/mach_error.h>
#include <stdio.h>
#include "IOKitHeader.h"
// Disables iOS's kernel watchdog, so you can unload watchdogd.
int main() {
  // based on watchdogd's entrypoint and power listener
  kern_return_t err;
  CFMutableDictionaryRef matching_dict = IOServiceMatching("IOWatchdog");
  io_service_t service =
      IOServiceGetMatchingService(kIOMasterPortDefault, matching_dict);
  if (!service) {
    fprintf(stderr, "Failed to discover watchdog service\n");
    return 1;
  }
  io_connect_t connection;
  err = IOServiceOpen(service, mach_task_self_, /*type=*/1, &connection);
  if (err) {
    fprintf(stderr, "IOServiceOpen failed with error: %s\n",
            mach_error_string(err));
    return 1;
  }
  err = IOConnectCallScalarMethod(connection, 3, 0, 0, 0, 0);
  if (err) {
    fprintf(stderr, "Failed to disable watchdog: %s\n", mach_error_string(err));
    return 1;
  }
  return 0;
}
