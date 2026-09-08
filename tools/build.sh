#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/bootstrap.py
tools/build-deps.sh
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/.cache/arm-webos-linux-gnueabi_sdk-buildroot/share/buildroot/toolchainfile.cmake" \
  -DBUILD_RECEIVER=ON
cmake --build build --parallel 6
python3 tools/package.py
