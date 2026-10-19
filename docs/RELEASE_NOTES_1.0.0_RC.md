# UxAir 1.0.0 release candidate

This is an installable candidate build for rooted compatible LG webOS TVs. It is not a final public release. The GitHub repository has not been renamed, and no GitHub Release or version tag is created by the candidate build workflow.

## Highlights

- H.264 mirroring through 1080p/60 and HEVC/H.265 mirroring through 4K/60, confirmed by the owner on an LG OLED B7 with webOS 3.9.
- Portrait/landscape adaptation, mirroring audio, Magic Remote navigation, local pairing/settings, diagnostics, and decoder recovery.
- Standalone RAOP audio playback path for negotiated AAC-LC, AAC-ELD, ALAC, and PCM converted to stereo PCM for Starfish. The owner confirmed standalone music/audio-only playback on the B7 in the tested workflow; individual codec variants were not tested separately.
- New UxAir app ID `com.github.depius.uxair`; the existing UxPlay development app remains installed and untouched.

## Installation and rollback

Install the IPK as `com.github.depius.uxair` using `ares-install`. It creates separate app-private pairing, identity, and settings data. The existing UxPlay development ID remains installed; do not uninstall it until UxAir has passed your checks. Roll back by closing UxAir and launching the retained UxPlay development app. No pairing/settings migration or removal is performed.

The candidate IPK, corresponding source archive, SHA-256 sums, ELF report, and source commit are uploaded as one GitHub Actions artifact. Verify the checksum before installation. Installation requires a compatible rooted TV and native development access.

## Limitations

- Standalone music/audio-only playback was confirmed on the B7 in the tested workflow. Individual negotiated audio codecs, all Apple Music protocol variants, protected media, and AirPlay 2 were not separately verified or claimed.
- No claim is made about HDR10/Main10, native 4K panel scanout, or displayed frame rate.
- The sender selects the actual AirPlay codec/rate; capability advertisement cannot force a particular choice.
- Only the LG OLED B7 with webOS 3.9 has the described physical validation. Other models and firmware are unverified.
- The Buy Me a Coffee account is not configured, so the candidate has no donation URL or badge.

## Validation status

The host test suite, ARMv7 build, ELF ABI check, package identity check, and package-content check passed in GitHub Actions for this candidate. CI does not establish hardware behavior; the owner separately confirmed standalone music/audio-only playback and the other tested workflows on the B7.
