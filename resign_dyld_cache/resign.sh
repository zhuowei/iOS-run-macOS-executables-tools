#!/bin/bash
set -e
python3 remove_signature.py "$1"
/usr/bin/codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	"$1"
python3 fixup_signature_size.py "$1"
