# Milestone 6.1: HEVC AirPlay startup correction

## B7 finding and cause

The owner's first receiver test negotiated HEVC and accepted the Starfish `H265` Load, but received about 30 HEVC frames/s while video Feed remained at zero. The queue reached 161 packets and the eight-second startup watchdog selected the known-good H.264 720p recovery. The receiver had an HEVC-only `!media.ready` check **before** validating and feeding its first access unit. That check repeatedly held the random-access frame while new packets queued.

In the [StarfishMediaAPIs header](https://www.webosbrew.org/webos-userland/d3/dda/StarfishMediaAPIs_8h_source.html), `0x11` is `PF_EVENT_TYPE_STR_RESOURCE_INFO`; `LOADCOMPLETED` is `0x16` and `PLAYING` is `0x1a`. The B7 excerpt contains `0x11`, but no `0x16`. The receiver's callback classification was correct: the resource event does not prove Load completed. The successful isolated HEVC probe accepts bounded Feed input after `Load()` returns true, even before `LOADCOMPLETED`, while requesting Play only after that callback. The missing callback's deeper cause remains unknown; HEVC plus PCM initialization is one possibility that this receiver-only test cannot establish.

The initial HEVC SPS was 996×2160; the sender's later 3840×2160 geometry message expresses sender geometry and is not proof that the incoming SPS changed. Decoder reinitialization remains tied to a parsed SPS/parameter-set change at a valid random-access frame. The initial frame is held through the existing 200 ms PTS preroll, then revalidated and fed. A one-frame portrait HEVC fixture followed by a UHD keyframe tests the resolution-change parser path. The existing 16 MiB receiver queue and 2 MiB access-unit bound remain unchanged.

## Correction

After a successful Starfish Load, the receiver permits **validated** HEVC access units through Feed before `LOADCOMPLETED`, up to 60 accepted frames or 8 MiB. It continues to use the existing Starfish callback to attach the ACB sink and request Play; Feed acceptance is not interpreted as decode or display success. When the preroll budget is exhausted, Feed waits for `LOADCOMPLETED`. Starfish errors, Feed rejection/backpressure, or the unchanged eight-second startup deadline still close the session and restore H.264 720p for a new connection. H.264 does not use this HEVC gate.

`[codec-start]` records Load acceptance, a first missing keyframe or malformed parameter set, the initial PTS wait, first accepted video Feed, a preroll-budget wait and a timeout stage with queue depth. `[media-state]` identifies resource information, Load completion and Playing callbacks. Existing `[media-init]` messages show Starfish Play requests and ACB loaded/playing transitions. These markers are bounded; they do not log compressed payloads.

## Validation and limits

`tools/test.sh` includes sanitizer-backed mock tests for delayed Load completion, Feed progress, finite preroll, error handling and reset, plus a generated 996×2160 portrait HEVC keyframe followed by a 3840×2160 parameter-set change. Existing H.264 settings, recovery, audio, pairing and diagnostics tests remain in the suite. GitHub Actions runs the Ubuntu tests, then the macOS ARMv7 build with ELF and IPK validation. A passing build does **not** establish B7 playback or native UHD output. The first successful receiver Feed, `0x16`, `0x1a`, decoder callbacks and visible picture must be checked on the television.

## Manual B7 retest

Keep the Milestone 6 development IPK for rollback. Upgrade the **development** app without uninstalling it so private pairing/settings data remain intact. Start with H.264 720p/30, then confirm H.264 1080p/30 and stereo audio still work. Disconnect, select experimental HEVC at 4K/30, reconnect the iPhone and mirror a stable SDR test pattern. Record the sanitized lines containing `[codec-start]`, `[media-state]`, `[media-init]`, `[codec]`, `[video]`, `[pipeline]`, `[session]` and `event=0x16`/`event=0x1a`. Do not share pairing or device-identifying log lines. A successful retest needs nonzero HEVC video Feed, a real Load/Play callback transition and stable playback; Feed alone is input acceptance only. Check orientation changes and an H.264 reconnect. Do not infer native 4K panel presentation from SPS, video-info or the requested ACB window.

If HEVC fails, the receiver selects H.264 720p for a new sender connection. If needed, reinstall the saved Milestone 6 development IPK over the development app, without uninstalling or changing the production app. This build does not deploy itself or alter firmware libraries.
