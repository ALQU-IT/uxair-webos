#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/bootstrap.py
cmake -S . -B build/hevc-cross -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/.cache/arm-webos-linux-gnueabi_sdk-buildroot/share/buildroot/toolchainfile.cmake" \
  -DBUILD_RECEIVER=OFF -DBUILD_HEVC_PROBE=ON
cmake --build build/hevc-cross --target hevc-probe --parallel 6
tools/generate-hevc-samples.sh build/hevc-samples
python3 tools/package-hevc-probe.py
