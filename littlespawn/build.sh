#!/bin/sh
set -e
clang -Os -target arm64-apple-ios12.0 -Wall \
	-isysroot "$(xcrun --sdk iphoneos --show-sdk-path)" \
	-o littlespawn \
	littlespawn.c
/usr/bin/codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	--entitlements littlespawn.entitlements \
	littlespawn
