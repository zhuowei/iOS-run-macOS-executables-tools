This is supposed to override the TASK_DYLD_INFO to (hardcoded) macOS 12b1 address.

doesn't work (lldb still doesn't load libraries) - can someone help me figure out why?

```
DYLD_INSERT_LIBRARIES=/usr/local/zhuowei/libtask_dyld_info_override.dylib debugserver 127.0.0.1:3335 --attach=vztool
```
