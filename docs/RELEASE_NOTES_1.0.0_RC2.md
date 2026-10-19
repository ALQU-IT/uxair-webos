# UxAir 1.0.0 release candidate 2

RC2 adds the final user-facing pairing and donation panels while retaining the tested RC1 receiver behavior. It remains a candidate for physical LG B7 validation; it is not a final public release.

## Changes

- About now shows UxAir 1.0.0, AirPlay Receiver for webOS, developer and UxPlay credits, the current GitHub repository, GPL-3.0-or-later, and the exact selectable row **Enjoying the app? Buy me a coffee.**
- Selecting the donation row opens a dedicated panel with an offline QR, the confirmed `buymeacoffee.com/depius` address, and a Back hint. The owner-provided image was not included in the supplied RC2 attachment or workspace, so the bundled 407×407 BMP is generated deterministically for `https://buymeacoffee.com/depius`, with a four-module quiet zone.
- The GitHub README uses a local badge linked to the confirmed creator profile, and `.github/FUNDING.yml` configures `buy_me_a_coffee: depius`.
- PIN pairing now has a centered high-contrast overlay. It stays visible while pairing is pending, updates when regenerated, clears after key registration or receiver reset, and takes display priority over the donation panel.
- Pairing and donation screens use local assets only and do not interrupt the Starfish playback session.
- If video starts in a standalone audio session, the receiver closes the audio-only pipeline while preserving the video keyframe and transitions into the normal mirroring startup path.

## Validation status

The RC2 workflow checks the QR generator output, tests pairing modal state and navigation, validates that the exact bitmap is included in the IPK, and runs host tests plus the ARMv7/ELF build. These checks do not verify QR scanning, PIN readability, or standalone Apple Music playback on the B7; the owner later confirmed the tested standalone music workflow on the final hardware-verified baseline.

## Known limits

- At the time RC2 was prepared, standalone Apple Music audio was pending B7 verification. The owner later confirmed standalone music/audio-only playback on the final B7-tested baseline; individual negotiated codec variants were not tested separately. The bundled RAOP API does not expose a standalone audio pause callback; flush is used as a temporary pause signal and the next audio packet resumes playback.
- HDR10/Main10, native 4K panel scanout, and displayed FPS are not claimed.
- iOS chooses the negotiated codec and rate; HEVC cannot reliably be forced at 720p/1080p.
- The physical validation described for this project is on the LG OLED B7 with webOS 3.9; other models remain unverified.
- The GitHub repository has not been renamed and no final release or tag is published.
