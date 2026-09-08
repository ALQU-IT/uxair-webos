# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-09-07

### Added

- Experimental native AirPlay receiver for rooted webOS TVs.
- H.264 mirroring, stereo PCM/AAC/ALAC audio handling, PIN pairing, AirPlay discovery, and adjustable audio delay.
- Reproducible local build, ABI verification, source-package generation, sanitizer-backed host tests, and release documentation.

### Known limitations

- Validated only on the LG 32LJ600B-SA with firmware 06.10.60.
- DRM/HLS, HEVC/HDR, recording, startup at boot, and broad-device compatibility are unsupported or unverified.
