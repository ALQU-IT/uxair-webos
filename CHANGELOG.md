# Changelog

All notable changes to this project are documented in this file.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.0] - Release candidate

### Added

- UxAir application identity, launcher branding, and 80/256/512 px icon assets, packaged under a new app ID to preserve existing UxPlay installations and pairing data.
- Standalone RAOP audio playback using the existing Starfish BUFFERSTREAM and PCM output path for negotiated AAC-LC, AAC-ELD, ALAC, and PCM streams.
- Audio session negotiation and lifecycle diagnostics, volume/mute handling, existing RAOP pause/flush/reconnect handling, and focused audio-mode tests.
- A large pairing PIN overlay, success/cancel/reset lifecycle state, a dedicated Buy Me a Coffee QR panel, and deterministic QR asset checks.
- Confirmed Buy Me a Coffee profile integration in the About page, README badge, and GitHub funding metadata.
- UxAir release documentation, source-package instructions, ARMv7 RC artifact workflow, checksum generation, and separate Audio session history.

### Changed

- Verified HEVC and 60 FPS options no longer carry obsolete experimental labels; sender codec/rate selection and unmeasured display FPS remain accurately described.
- Replaced the legacy UxPlay launcher identity with `com.github.depius.uxair`; old development installations are retained as a separate app and are never removed automatically.
- Updated public metadata, executable naming, packaging, and CI artifact names for UxAir.

### Limitations

- Standalone AirPlay audio is implemented but awaits physical LG B7 validation; protected streams, unsupported protocols, and AirPlay 2 are not modified or claimed.
- Standalone AirPlay audio still awaits physical LG B7 validation, including Apple Music and sender pause/resume behavior.
- The original owner-provided QR image was not present in the RC2 attachment or workspace; the bundled offline image is generated for the confirmed profile URL.
- HDR10/Main10, native 4K scanout, and output/display FPS measurement remain unavailable.

## [Unreleased]

### Fixed

- Compose pairing and donation dialogs over the active UI in a single SDL frame; pairing Back restarts the RAOP listener to abandon the pending authentication attempt.
- Preserve pairing overlays across redraws and avoid repeated presentation while a pairing PIN remains unchanged.

### Changed

- Offer 720p, 1080p and 4K independently of the selected codec, with valid atomic H.264/HEVC profile transitions.
- Simplify codec labels and use the UxAir navy/blue theme, updated launcher artwork and RC3 package branding.

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
