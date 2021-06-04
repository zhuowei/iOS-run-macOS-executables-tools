#!/bin/bash
set -e
python3 remove_signature.py "$1"
/usr/bin/codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	"$1"
python3 fixup_signature_size.py "$1"
target_path="/usr/local/zhuowei/dyld_shared_cache_arm64e"
sha256_filename="$(echo -n "$target_path" | shasum -a 256 | cut -f 1 -d " ")"
codesign -d -vvv "$1" 2>&1 |grep "^CDHash="|cut -f 2 -d "="|xxd -r -p >"$sha256_filename"
echo "put $sha256_filename in /taurine/cstmp/"
