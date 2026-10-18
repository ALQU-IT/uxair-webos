# UxPlay webOS

An experimental native AirPlay mirroring receiver for rooted LG webOS TVs. Open **UxPlay webOS** from the TV launcher, select it from Screen Mirroring on an iPhone, iPad, or Mac, then enter the on-screen PIN on first connection. No separate receiver computer is needed.

## Status and validated hardware

This is an experimental `0.1.0` release. It has been validated on an LG **32LJ600B-SA**, firmware **06.10.60**, Rockhopper **3.9.3**, with an ARMv7 Cortex-A9 target. On an LG OLED B7 / Rockhopper 3.9.0, H.264 1080p/30 and experimental HEVC 4K/30 AirPlay mirroring, Photos/video through mirroring, stereo audio, and HEVC `LOADCOMPLETED`/`PLAYING` callbacks have been confirmed. The default mirroring profile remains 1280×720 / 30 fps. HEVC is opt-in; native 4K output resolution and HDR10/Main10 remain unverified and disabled. See [the validation record](docs/VALIDATION.md), [B7 pacing record](docs/MILESTONE2.md), and [capability assessment](docs/CAPABILITIES.md).

## Requirements

- A rooted compatible LG webOS TV with the webOS Brew development environment.
- The TV and sender on the same LAN with multicast/mDNS available.
- For the current ARMv7 package build: Apple Silicon macOS, Xcode command-line tools, Git, CMake, Ninja, and the webOS TV CLI (`ares-package`, `ares-install`, `ares-launch`). The checked-in target SDK pin selects macOS ARM64; pinned FFmpeg source is cross-compiled by the build.
- For native tests on Ubuntu or WSL2 Ubuntu: a C++11 compiler, FFmpeg, pkg-config, and FFmpeg development packages. See [development environments](docs/DEVELOPMENT.md).

## Install and use

Build the package locally, then configure a neutral device name with `ares-setup-device`:

```sh
tools/build.sh
TV_DEVICE=your-tv tools/tv.sh install
TV_DEVICE=your-tv tools/tv.sh launch
```

The package is `dist/com.github.danlflh.uxplaywebos_0.1.0_arm.ipk`. Changing from the former development ID requires manually removing that old build; pairing data belongs to the app ID, so it starts over after the identity change.

On the TV, open **UxPlay webOS**. Start Screen Mirroring from an iPhone, iPad, or Mac and select **UxPlay webOS**. Enter the PIN when prompted. On macOS, select **UxPlay webOS** as the audio output if it is not selected automatically.

- In the development build, Left/Right changes settings section, Up/Down focuses a row, OK opens a choice dialog, and Back cancels it. Set audio delay from **System Settings** before connecting; positive values play audio later. Settings are saved locally.
- During playback, OK opens the diagnostics page; Back hides it, then Back again disconnects. Back from the idle Screen Mirroring section, or Exit, closes the app.
- One transmitting device is accepted at a time.
- The development app has five settings sections, local usage statistics and diagnostics. See [Milestone 4 controls and TV validation](docs/MILESTONE4.md). Direct media remains unavailable; HEVC mirroring is experimental and native 4K output and HDR are unverified.
- The development package includes a bounded, metadata-only [direct-media request probe](docs/MILESTONE4C.md). Its [temporary `/info` handshake experiment](docs/MILESTONE4D.md) answers only a bodyless HTTP `GET /info`; it does not enable direct playback or change advertised capabilities.
- The [Milestone 4E transport and native-resolution audit](docs/MILESTONE4E.md) records why a direct 4K photo prototype remains blocked pending a real payload trace, a safe authorization boundary and a verified B7 output surface.
- The [Milestone 5A HEVC mirroring audit](docs/MILESTONE5.md) records the pinned transport path. HEVC is now an opt-in development setting; see [Milestone 6](docs/MILESTONE6.md) and its follow-up notes.
- The [Milestone 5B read-only firmware discovery procedure](docs/MILESTONE5B.md) identifies the evidence needed before any standalone HEVC hardware probe is built.

For direct SSH helpers, explicitly set the connection details—no personal network, TV alias, or key path is embedded:

```sh
export TV_DEVICE=your-tv
export TV_HOST=tv.example.lan
export TV_KEY=/absolute/path/to/private-key
tools/tv.sh logs
tools/tv.sh loader-check
```

`tools/tv.sh update-binary` replaces only the executable and intentionally preserves current pairing data. Use a full install whenever packaged libraries change.

## Reproducible build and tests

`packaging/appinfo.json` is the single version source. `tools/build.sh` downloads checksum-pinned inputs, builds private dependencies, cross-compiles the receiver, verifies the ELF ABI, and packages the IPK. The generated dependencies, build tree, logs, pairing state, and `dist/` artifacts are excluded from Git.

```sh
tools/test.sh              # H.264, malformed input, PCM/AAC and ASan/UBSan
tools/test.sh --diagnostics-only # standalone diagnostics timing test
tools/probe.sh             # package a 15-second native playback probe
tools/source-package.py    # corresponding source archive
python3 tools/validate-metadata.py
```

The corresponding source archive is `dist/uxplay-webos-0.1.0-source.tar.gz`; it includes this source tree, pinned UxPlay source, original dependency archives, and build instructions. Native test CI runs on Ubuntu 24.04. The separate Apple Silicon workflow builds an isolated development IPK; see [download, installation, and rollback instructions](docs/DEVELOPMENT-IPK.md) and [development environment findings](docs/DEVELOPMENT.md).

## Architecture

`src/receiver.cpp` integrates the pinned UxPlay core and handles discovery, PIN pairing, and bounded packet queues. `src/main.cpp` owns session lifecycle and timing. `src/media.cpp` feeds the TV hardware decoder through Starfish Media Pipeline and ACB, while FFmpeg decodes AAC-ELD, AAC-LC, ALAC, and PCM to stereo PCM. SDL owns the window and remote input while the hardware video plane remains visible.

Queues are capped at 4 MiB/512 packets in the default 720p profile and 8 MiB/512 packets in the B7-tested 1080p profile. Decoder stalls, timestamp discontinuities, and queue overflow end the current session rather than allowing uncontrolled memory growth.

Every five seconds, `uxplay.log` records lightweight diagnostics while a session is active: incoming H.264 access-unit FPS, FPS and interval spread derived from remote presentation timestamps, accepted Starfish feed rate, decoder callback rate, feed retries and call latency, queue occupancy/high-water marks and drops, audio decode time/errors, process RSS, and session/reset counts. The incoming rate is measured at packet ingress. `FRAMEREADY` is a throttled firmware callback, so its reported rate is an activity signal rather than a precise hardware FPS measurement. Queue limits and playback behavior are unchanged by diagnostics.

## Privacy, security, and logs

Pairing data, receiver identity, and the selected audio delay are stored only in the app's private `state/` directory. Up to 64 paired client keys are retained. Reinstalling or removing app data resets that state. The receiver has no cloud service, analytics, or outbound account connection.

Diagnostics are written to `uxplay.log`, rotated around 1 MiB with one previous file. Exporting logs does not copy pairing files. Do not publish logs without review: sender names and local network details may appear in them. The package contains no SSH keys, logs, or pairing data.

## Supported features and limitations

Supported: AirPlay discovery, PIN pairing, H.264 mirroring, stereo audio with mirroring, iPad rotation, reconnecting, and adjustable audio delay. Experimental HEVC 4K/30 mirroring has been verified on the LG OLED B7.

Not supported or not certified: standalone audio-only playback, direct photo/video and HLS playback, full AirPlay 2, native 4K output resolution, HDR10/Main10 playback, recording, startup at boot, broad TV/firmware compatibility, Mac extended display, and low-latency gaming. Starfish buffering on this older platform can add visible latency.

## Troubleshooting

- **The receiver does not appear:** confirm both devices use the same LAN and that multicast/mDNS is not blocked.
- **No sound on a Mac:** choose **UxPlay webOS** in macOS audio output, reconnect, then adjust audio delay if needed.
- **Old app still appears or pairing changed:** remove the old development-ID installation manually; this release intentionally uses a new app ID.
- **Package or loader failure:** run `tools/tv.sh loader-check` with `TV_HOST` and `TV_KEY` set, and inspect `tools/tv.sh logs`.

## Roadmap and contributing

Before a stable release, the project needs longer soak tests, quantified latency, more TV/firmware coverage, Mac audio and extended-display validation, and repeated reconnect/network-recovery testing. Contributions are welcome as focused issues and pull requests with reproducible evidence. Please run `tools/test.sh` and `python3 tools/validate-metadata.py` before opening a pull request.

## Credits and license

UxPlay webOS is maintained by Daniel Filho ([@danlflh](https://github.com/danlflh)). It builds on FDH2/UxPlay, llhttp, OpenSSL, libplist, FFmpeg, the GCC Runtime, the webOS Brew native toolchain and samples, and technical reference from mariotaku/ss4s. SDL2, SDL2_ttf, Starfish Media Pipeline, and ACB are supplied by the TV. Full notices are in [THIRD_PARTY.md](THIRD_PARTY.md).

Licensed under [GPL-3.0-or-later](LICENSE). This is an independent project and is not affiliated with, endorsed by, or supported by Apple or LG.
