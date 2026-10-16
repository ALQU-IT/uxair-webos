#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
tools/generate-hevc-samples.sh build/hevc-native-samples
mkdir -p build/tests
c++ -std=c++11 -g -O1 -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc \
  tests/hevc_sample_test.cpp src/hevc_sample.cpp -o build/tests/hevc-sample-test
build/tests/hevc-sample-test build/hevc-native-samples/sample-1080p.hevc build/hevc-native-samples/sample-uhd.hevc
echo 'PASS: sanitizer-backed HEVC Main 8-bit Annex B parser and rejection cases'
c++ -std=c++11 -g -O1 -Wall -Wextra -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc \
  tests/hevc_launch_test.cpp src/hevc_launch.cpp -o build/tests/hevc-launch-test
build/tests/hevc-launch-test
echo 'PASS: sanitizer-backed native webOS launch parsing and idle safety'
