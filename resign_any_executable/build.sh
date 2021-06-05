#!/bin/sh
set -e
lipo -thin arm64e -output "$2" "$1"
chmod +w "$2"
codesign -d --entitlement "$2.entitlement" "$2"
python3 set_to_arm64.py "$2"
if [[ -s "$2.entitlement" ]]
then
codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	--entitlement "$2.entitlement" \
	"$2"
else
codesign --force \
	--sign 52F754C59CFAC59BC5794F5A3B523EFE0667D7EB \
	--timestamp=none \
	"$2"
fi
