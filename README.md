<p align="center"><img src="assets/uxair-icon-256.png" width="128" alt="UxAir receiver icon"></p>

<h1 align="center">UxAir</h1>
<p align="center"><strong>AirPlay Receiver for webOS</strong></p>

UxAir is a native AirPlay mirroring receiver for compatible rooted LG webOS TVs, based on the open-source UxPlay project. It is an independent community project and is not affiliated with or endorsed by Apple or LG. UxAir is compatible with AirPlay; AirPlay is an Apple trademark.

## Status

Version 1.0.0 is prepared as a release candidate. The verified physical target is the LG OLED B7 (2017) running webOS 3.9. Testing on other models or firmware is not implied. HDR10/Main10 is not supported. Native 4K output-plane resolution and displayed FPS are not measured by this application.

## Features

- H.264 AirPlay mirroring up to 1080p/60.
- HEVC/H.265 mirroring up to 4K/60.
- 30 and 60 FPS profiles, including the confirmed 30 FPS modes.
- Portrait and landscape adaptation.
- Starfish hardware-accelerated video playback with stereo audio during mirroring.
- Configurable codec, resolution, frame rate, audio delay, and diagnostics.
- Magic Remote pointer, arrow, OK, and Back navigation.
- Large on-screen AirPlay PIN pairing, saved configuration, session history, and automatic decoder recovery/fallback.
- Lightweight diagnostics separating advertised rate, incoming rate, accepted Feed rate, and decoder callbacks. Decoder callback rate does not claim displayed FPS.

An audio-only RAOP path for negotiated AAC, AAC-ELD, ALAC, and PCM is implemented but awaits physical B7 validation; it is not listed as a verified feature.

The iPhone, iPad, or Mac selects the actual AirPlay codec and rate. Selecting HEVC advertises HEVC alongside H.264; it cannot force a sender that chooses H.264. Direct Photos playback is distinct from Photos or video shown by mirroring. Direct Photos/video, HLS, AirPlay 2, HDR/Main10, and native 4K scanout are not supported or verified.

## Screenshots

Owner-provided screenshots will be added here after the final launcher artwork and release candidate are reviewed. No screenshots are included yet.

## Hardware and requirements

- A compatible rooted LG webOS TV with native application development enabled.
- A TV and sender on the same local network with multicast/mDNS discovery available.
- Physical playback has been verified on the LG OLED B7 with webOS 3.9. Other TV models remain unverified.
- The ARMv7 build uses the pinned webOS Brew toolchain. CI performs host tests and builds/validates the ARMv7 IPK.

## Install and use

Download the `com.github.depius.uxair_1.0.0_arm.ipk` candidate from the GitHub Actions artifact, then install it on a compatible rooted TV with the webOS CLI:

```sh
ares-install -d YOUR_TV com.github.depius.uxair_1.0.0_arm.ipk
ares-launch -d YOUR_TV com.github.depius.uxair
```

Open UxAir, select it from Screen Mirroring on the sender, and enter the large on-screen PIN when prompted. For music, select UxAir as the sender's AirPlay audio output. Pairing, receiver identity, and settings are private to the app ID.

### Migrating from UxPlay webOS development builds

UxAir uses the new ID `com.github.depius.uxair`. It installs alongside the existing UxPlay webOS development app and does not overwrite or uninstall it. The new ID has separate pairing keys, receiver identity, and settings; you will pair again and configure UxAir separately. Keep the old development app installed until the UxAir candidate has passed your hardware checks. No state is migrated or removed automatically.

## Remote and configuration

- Left/Right changes the Video, Audio, Diagnostics, and About sections.
- Up/Down focuses rows; OK opens or confirms a choice; Back cancels a dialog or pairing attempt, returns from playback to the UI, or exits from the idle screen.
- During playback, OK opens diagnostics and Back returns to video.
- Set audio delay before connecting; positive values delay the TV audio.
- Choose 30 or 60 FPS as the advertised maximum. The sender may use a lower rate.
- HEVC capability does not force the sender's codec selection.

## Build from source

`packaging/appinfo.json` holds the app identity and version. Inputs and their checksums are pinned in `dependencies.lock.json`. Follow [the development environment guide](docs/DEVELOPMENT.md) and use a supported macOS ARM64 host with the webOS CLI for the target build.

```sh
python3 tools/validate-metadata.py
tools/test.sh
tools/build.sh
python3 tools/validate-ipk.py
tools/source-package.py
```

The build verifies ARM32 soft-float ABI and GLIBC compatibility. The source archive contains this application source, the pinned UxPlay source, dependency archives, notices, and build instructions. CI validation does not replace physical TV testing.

## Known limitations and troubleshooting

- Audio-only playback has not yet been confirmed on the B7. Protected media and unsupported AirPlay protocols are not decrypted or altered.
- An AirPlay sender may select a different codec or frame rate than the requested maximum.
- Incoming/Feed rates and Starfish `FRAMEREADY` callbacks are not proof of displayed FPS. Native panel output resolution is unmeasured.
- HDR10/Main10, direct Photos/video, AirPlay 2, startup at boot, broad TV compatibility, and macOS extended-display use are not supported or certified.
- Starfish buffering on webOS 3.9 can add latency.
- If the receiver does not appear, check same-network connectivity and multicast/mDNS. If a codec fails, reconnect and choose a supported profile. Review `uxair.log`; logs can contain sender names and local network details.

## Acknowledgements and license

This project builds on the excellent work of:

- **UxPlay by FDH2** — the original open-source AirPlay mirroring server.
- [**uxplay-webOS by Daniel Filho (@danlflh)**](https://github.com/danlflh/uxplay-webos) — the original webOS port.

UxAir extends this foundation with additional features, performance improvements, and a redesigned interface. Development and adaptation: [@depius](https://github.com/depius). This repository is [depius/uxair-webos](https://github.com/depius/uxair-webos) (formerly `uxplay-webos`). UxAir is licensed under [GPL-3.0-or-later](LICENSE); corresponding sources, upstream attribution, and third-party notices are preserved in [THIRD_PARTY.md](THIRD_PARTY.md).

## Support development

<a href="https://buymeacoffee.com/depius"><img src="assets/buy-me-a-coffee.svg" alt="Buy me a coffee"></a>

UxAir is free and open source. If you enjoy using it, you can support its development by buying me a coffee.

https://buymeacoffee.com/depius

Donations are optional and do not unlock features or affect the license. The app displays a bundled offline QR code for the same URL; the owner-provided QR image was not included with the supplied RC2 files, so this candidate uses a deterministic QR generated locally for the confirmed profile.
