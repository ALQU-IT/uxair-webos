# UxAir 1.0.0 Release Candidate

This release candidate starts from the owner hardware-verified RC5 playback baseline. The final UI batch changes launcher artwork, app tile color metadata, branding documentation, and packaging validation; it does not change media, pairing, RAOP, ACB, Starfish, H.264/HEVC, frame-rate, or audio implementation.

## UI and repository changes

- The launcher artwork uses a full-bleed `#1D2D47` background with the existing centered light AirPlay-style mark. Small and large webOS icons and repository previews are generated from the SVG master.
- The app manifest specifies the matching tile color and concise subtitle `AirPlay Receiver for webOS`.
- The About screen credits the original UxPlay core and webOS port by their project names and handles; developer identity remains `@depius`.
- The README includes tested hardware, feature limits, installation steps, credits, licensing, optional support, and a placeholder for screenshots that the owner will provide.
- The repository description, topics, and rename instructions are prepared. No repository settings, release, or tag were changed.

## Hardware verification

The owner confirmed these workflows on the physical LG OLED B7 (2017), rooted webOS 3.9:

- H.264 mirroring through 1080p/60 and HEVC 4K/60 SDR mirroring.
- Photos and videos shown through the tested mirroring workflows, including portrait/landscape transitions.
- Standalone AirPlay music/audio-only playback and audio during mirroring. Individual negotiated audio codec variants were not tested separately.
- PIN pairing, Back cancellation, reconnection with saved credentials, resolution/codec/frame-rate selection, and Magic Remote navigation.
- Final slate-blue launcher icon and navy/blue application interface.

Native 4K output-plane resolution and displayed FPS remain unmeasured. HDR10/Main10, direct Photos/video protocols, AirPlay 2 certification, and other LG TV models are not claimed.

## Validation scope

GitHub Actions passed host sanitizer-backed tests, the ARMv7 IPK build, ELF/IPK validation, and source archive generation for the release-candidate commit. CI does not itself verify physical TV behavior.
