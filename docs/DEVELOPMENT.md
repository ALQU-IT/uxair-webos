# Development and build environments

## Native tests on Linux or WSL2 Ubuntu

The C++ unit tests run on the build host and do not need the webOS SDK or TV libraries. The diagnostics timing test needs only a C++ compiler with C++11 and AddressSanitizer/UndefinedBehaviorSanitizer support. The media tests also need FFmpeg's command-line encoder and development libraries.

On Ubuntu 24.04 (including WSL2 Ubuntu), install the prerequisites:

```sh
sudo apt-get update
sudo apt-get install --yes build-essential ffmpeg pkg-config libsdl2-dev libavcodec-dev libavutil-dev libswresample-dev
```

Run just the diagnostics test:

```sh
tools/test.sh --diagnostics-only
```

Run the complete native test suite, including SDL remote input mapping, H.264 landscape/portrait parsing, malformed input, PCM validation, and AAC-LC decoding:

```sh
tools/test.sh
```

Check package metadata separately:

```sh
python3 tools/validate-metadata.py
```

The GitHub Actions `host-tests` job uses Ubuntu 24.04 and runs metadata validation plus the complete sanitizer-backed native tests. The [branch push run on 2026-10-14](https://github.com/depius/uxplay-webos/actions/runs/38603156648) passed all steps. The native tests do not build or exercise the TV application.

For WSL2, keep the checkout in the Linux filesystem (for example `~/src/uxplay-webos`) for normal Linux file permissions and build performance. This workspace currently has no WSL distribution or container runtime installed; the commands above have not been executed here.

## ARMv7 webOS target build

The deployed target remains 32-bit ARM EABI soft-float with a glibc ceiling of 2.19. `tools/verify-elf.py` checks the ELF class/machine/float ABI, GLIBC symbol requirements, bundled C++ runtime symbol coverage, `$ORIGIN/lib`, and forbidden dynamic dependencies. `tools/package.py` stages the executable and the private `libstdc++.so.6`; SDL2, SDL2_ttf, Starfish Media Pipeline, and ACB remain TV-provided. The package does not install or replace system libraries on the TV.

The checked-in lock pins the `webosbrew/native-toolchain` **macOS ARM64** (`darwin-arm64`) archive at release `webos-d7ed7ee.6`, with its SHA-256. It cannot execute on Linux. The [assets at that exact release](https://github.com/webosbrew/native-toolchain/releases/tag/webos-d7ed7ee.6) include Linux **AArch64**, macOS ARM64, and macOS x86_64 hosts, but no Linux x86_64 host SDK for ordinary WSL2 Ubuntu or an x86_64 Linux container. The release metadata does not publish an asset digest for the Linux AArch64 archive. The [openlgtv/buildroot-nc4 release](https://github.com/openlgtv/buildroot-nc4/releases/tag/webos-d7ed7ee) has a differently packaged `arm-webos-linux-gnueabi` SDK, but its host architecture, checksum, sysroot equivalence, and compatibility with this project's pinned SDK have not been verified. It is not a build input for this repository.

The current `tools/build-deps.sh` uses macOS `shasum`; the packaging step needs the host-installed webOS CLI (`ares-package`). Consequently, the current project does not claim a reproducible Linux/WSL2 ARMv7 IPK build. Do not substitute a generic ARM cross-compiler or a newer glibc sysroot: that could produce an IPK incompatible with the LG B7.

The existing reproducible target-build path is Apple Silicon macOS with the checked-in toolchain pin, Xcode command-line tools, Git, CMake, Ninja, and the webOS TV CLI. From a clean checkout, run:

```sh
python3 tools/validate-metadata.py
tools/build.sh
python3 tools/verify-elf.py build/app
shasum -a 256 dist/*.ipk
```

`tools/build.sh` verifies download hashes before extraction, builds the pinned private dependencies, cross-compiles, runs ELF verification during packaging, and writes the IPK under `dist/`. The explicit second ELF check is a final preflight of the staged package. This path has not been rerun in the current Windows workspace; the prior hardware validation used Apple Silicon macOS. It produces a development IPK only on a host with those prerequisites.

The [Apple Silicon Actions workflow](../.github/workflows/armv7-ipk.yml) runs Ubuntu native tests first, then `tools/build.sh --development` with the same locked SDK and uploads a separately identified development IPK after metadata, ELF, and package checks. See [artifact download and manual installation instructions](DEVELOPMENT-IPK.md). The previous development IPK has been owner-tested on the LG B7; each new build still needs runtime validation.

To enable an Ubuntu target build, first obtain a Linux **x86_64** host SDK from a fixed release, independently record its SHA-256, inspect its target sysroot and `pkg-config` metadata against the pinned SDK, adapt the bootstrap and build scripts for the host, and validate a clean end-to-end build and `tools/verify-elf.py` result. A Linux AArch64 host is another possible route with the `.6` asset, but it needs a recorded checksum, a real AArch64 build environment, and the same validation. The artifact must retain the ARM32 soft-float and glibc 2.19 checks. The target media pipeline must continue linking Starfish and ACB and using TV-provided SDL2 libraries; native host test libraries are not substitutes for them.

## Runtime validation boundary

The earlier hardware record covers Rockhopper 3.9.3. The owner has also confirmed basic AirPlay playback on the LG OLED B7 with Rockhopper 3.9.0 using a development IPK, as recorded in [Milestone 2](MILESTONE2.md). A successful host test or ELF preflight cannot prove performance or compatibility of a new package; each new build still requires playback checks on the intended firmware. Deployment is a separate, explicit step.
