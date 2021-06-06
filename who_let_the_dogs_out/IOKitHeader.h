#pragma once
// copied from IOKitLib.h

#include <CoreFoundation/CFDictionary.h>
#include <mach/mach.h>

typedef mach_port_t     io_object_t;

typedef io_object_t     io_connect_t;
typedef io_object_t     io_service_t;

extern mach_port_t kIOMasterPortDefault;

CFMutableDictionaryRef
IOServiceMatching(
        const char *    name ) CF_RETURNS_RETAINED;

io_service_t
IOServiceGetMatchingService(
        mach_port_t     masterPort,
        CFDictionaryRef matching CF_RELEASES_ARGUMENT);

kern_return_t
IOServiceOpen(
        io_service_t    service,
        task_port_t     owningTask,
        uint32_t        type,
        io_connect_t  * connect );

kern_return_t
IOServiceOpen(
        io_service_t    service,
        task_port_t     owningTask,
        uint32_t        type,
        io_connect_t  * connect );

kern_return_t
IOConnectCallScalarMethod(
        mach_port_t      connection,            // In
        uint32_t         selector,              // In
        const uint64_t  *input,                 // In
        uint32_t         inputCnt,              // In
        uint64_t        *output,                // Out
        uint32_t        *outputCnt)             // In/Out
AVAILABLE_MAC_OS_X_VERSION_10_5_AND_LATER;
