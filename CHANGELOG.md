# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- Configurable advertised maximum of 30 or experimental 60 FPS for H.264 720p/1080p and HEVC up to 4K.
- Separate diagnostics for advertised maximum, incoming rate, PTS cadence, accepted Feed rate and decoder callbacks.

### Changed

- Persist the selected frame rate; remove the obsolete HDR preference while ignoring it when loading existing settings.
- Lower the advertised rate to 30 FPS after a 60 FPS decoder failure, with restoration of the previous receiver profile if recovery fails.

## [0.1.0] - 2026-09-07

### Added

- Experimental native AirPlay receiver for rooted webOS TVs.
- H.264 mirroring, stereo PCM/AAC/ALAC audio handling, PIN pairing, AirPlay discovery, and adjustable audio delay.
- Reproducible local build, ABI verification, source-package generation, sanitizer-backed host tests, and release documentation.

### Known limitations

- Validated only on the LG 32LJ600B-SA with firmware 06.10.60.
- DRM/HLS, HEVC/HDR, recording, startup at boot, and broad-device compatibility are unsupported or unverified.
