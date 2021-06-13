#!/bin/sh
set -e
clang -Os -target arm64e-apple-ios14.0 -Wall -shared \
	-isysroot "$(xcrun --sdk iphoneos --show-sdk-path)" \
	-o libtask_dyld_info_override.dylib \
	task_dyld_info_override.c
/usr/bin/codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	libtask_dyld_info_override.dylib
