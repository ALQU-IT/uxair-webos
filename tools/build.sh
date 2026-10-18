#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
case "${1:-}" in
  '') variant=production; cmake_variant=OFF ;;
  --development) variant=development; cmake_variant=ON ;;
  *) echo 'Usage: tools/build.sh [--development]' >&2; exit 2 ;;
esac
python3 tools/bootstrap.py
tools/build-deps.sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/.cache/arm-webos-linux-gnueabi_sdk-buildroot/share/buildroot/toolchainfile.cmake" \
  -DBUILD_RECEIVER=ON -DBUILD_DEVELOPMENT_PACKAGE="$cmake_variant"
cmake --build build --parallel 6
printf '%s\n' "$variant" > build/package-variant.txt
if [[ "$variant" == development ]]; then
  python3 tools/package.py --development
else
  python3 tools/package.py
fi
