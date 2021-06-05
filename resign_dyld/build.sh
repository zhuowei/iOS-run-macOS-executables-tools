#!/bin/sh
set -e
lipo -thin arm64e /usr/lib/dyld -output dyld_macos
python3 patch_dyld.py dyld_macos
/usr/bin/codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	dyld_macos
