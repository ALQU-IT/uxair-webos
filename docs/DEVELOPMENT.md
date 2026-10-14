# Development and build environments

## Native tests on Linux or WSL2 Ubuntu

The C++ unit tests run on the build host and do not need the webOS SDK or TV libraries. The diagnostics timing test needs only a C++ compiler with C++11 and AddressSanitizer/UndefinedBehaviorSanitizer support. The media tests also need FFmpeg's command-line encoder and development libraries.

On Ubuntu 24.04 (including WSL2 Ubuntu), install the prerequisites:

```sh
sudo apt-get update
sudo apt-get install --yes build-essential ffmpeg pkg-config libavcodec-dev libavutil-dev libswresample-dev
```

Run just the diagnostics test:

```sh
tools/test.sh --diagnostics-only
```

Run the complete native test suite, including H.264 landscape/portrait parsing, malformed input, PCM validation, and AAC-LC decoding:

```sh
tools/test.sh
```

Check package metadata separately:

```sh
python3 tools/validate-metadata.py
```

The GitHub Actions `host-tests` job uses Ubuntu 24.04 and runs metadata validation plus the complete sanitizer-backed native tests. The native tests do not build or exercise the TV application.

For WSL2, keep the checkout in the Linux filesystem (for example `~/src/uxplay-webos`) for normal Linux file permissions and build performance. This workspace currently has no WSL distribution or container runtime installed; the commands above have not been executed here.

## ARMv7 webOS target build

The deployed target remains 32-bit ARM EABI soft-float with a glibc ceiling of 2.19. `tools/verify-elf.py` checks the ELF class/machine/float ABI, GLIBC symbol requirements, bundled C++ runtime symbol coverage, `$ORIGIN/lib`, and forbidden dynamic dependencies. `tools/package.py` stages the executable and the private `libstdc++.so.6`; SDL2, SDL2_ttf, Starfish Media Pipeline, and ACB remain TV-provided. The package does not install or replace system libraries on the TV.

The checked-in lock currently pins the webOS SDK archive for **macOS ARM64** (`darwin-arm64`). That archive cannot be used as a Linux host toolchain. The webosbrew native-toolchain project publishes a Linux x86_64 port as well, and documents use of its relocated SDK with CMake, but this repository does not yet pin that Linux archive and its checksum. The Linux archive's checksum could not be obtained and verified in this environment. The current `tools/build-deps.sh` also contains macOS-specific `shasum` usage, while `tools/build.sh` and packaging assume a host-installed webOS CLI. Therefore the current project does not claim a reproducible Linux/WSL2 ARMv7 IPK build.

The supported target-build path remains Apple Silicon macOS with the exact checked-in toolchain pin and the prerequisite webOS CLI. An Ubuntu ARMv7 build should only be enabled after adding a host-specific, checksum-pinned Linux SDK input, making bootstrap/dependency/packaging scripts portable, and validating a clean end-to-end build plus `tools/verify-elf.py`. The artifact must retain the current ARM32 soft-float and glibc 2.19 checks. The target media pipeline must continue linking Starfish and ACB and using TV-provided SDL2 libraries; native host test libraries are not substitutes for them.

## Runtime validation boundary

The available hardware record covers Rockhopper 3.9.3. It does not establish Rockhopper 3.9.0 compatibility. A successful host test or ELF preflight cannot prove proprietary Starfish/ACB runtime compatibility; an IPK from a validated toolchain still requires loader and playback checks on the intended firmware. Deployment is a separate, explicit step.
