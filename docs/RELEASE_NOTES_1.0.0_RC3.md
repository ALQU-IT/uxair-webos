# UxAir 1.0.0 release candidate 3

RC3 addresses the B7 pairing overlay defect and finalizes the requested settings and visual cleanup. This remains a development candidate; it is not a public release.

## Changes

- Pairing and donation are now composed over the regular page or playback overlay in a single SDL presentation. The PIN panel is centered, opaque, and rendered above an alpha backdrop; repeated PIN callbacks do not trigger redundant redraws.
- Magic Remote Back cancels pending pairing by stopping and restarting the RAOP listener. Completed pairing keys and receiver identity remain persisted, and a later attempt can provide a fresh PIN.
- Resolution offers 720p, 1080p and 4K regardless of codec. Selecting 4K enables HEVC; switching to H.264 from 4K returns to 1080p. Codec/resolution settings remain validated and saved through the existing receiver restart path.
- The codec row uses `Video codec`, `H.264`, and `H.265 (HEVC)`. About credits display `@depius`, identify the project as UxAir, retain UxPlay/webOS attribution and GPL notices, and preserve the donation entry.
- Updated the launcher artwork and dark navy/blue UI palette. The offline donation QR remains on a white background.
- Updated repository presentation and CI artifact naming for `depius/uxair-webos` / RC3.

## Validation and limits

Host tests, metadata validation, the ARMv7 build, ELF ABI checks, and IPK-content checks are required in the RC3 Actions workflow. CI does not verify dialog appearance or key handling on a television. This session's GitHub tools do not expose repository rename or metadata settings; exact manual steps are in [repository metadata preparation](REPOSITORY_METADATA.md). HDR10/Main10 and native 4K panel output are not claimed.

## Targeted LG B7 checks

1. Verify 720p/1080p H.264 selection, then 720p/1080p/4K codec transitions and persisted settings after relaunch.
2. Trigger pairing, confirm the centered PIN stays steady, press Back, then start pairing again and confirm a new PIN is shown.
3. Complete pairing and verify H.264/HEVC mirroring with audio; confirm a later pairing attempt does not replace saved keys.
4. Open the About donation panel, scan the QR, press Back, and verify normal remote navigation resumes.
