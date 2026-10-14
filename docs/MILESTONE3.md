# Milestone 3, phase 1: settings, statistics and diagnostics

The [capability assessment](CAPABILITIES.md) is the source of truth for working and gated media paths. This phase preserves the B7-tested H.264 720p/30 profile. 1080p, 60 FPS, 4K, HEVC, direct media and HDR remain disabled. No media format or Starfish/ACB output mode has been changed.

## TV interface

### Magic Remote OK input

The AN-MR650 wheel press may arrive as an SDL left mouse button press. The
interface now treats `SDL_MOUSEBUTTONDOWN` with `SDL_BUTTON_LEFT` as Select,
alongside Return, keypad Enter, and SDL's standard Select key. Directional,
Back and Exit mappings are unchanged. A key press and left-button press within
120 ms of one another yield one Select; rapid clicks from the same event source
remain distinct. Auto-repeat of a held Select key is ignored.

The SDL webOS key mapping explicitly handles LG Back and Exit, but does not
define a dedicated LG OK scancode. The precise AN-MR650 event on Rockhopper
3.9.0 still needs a TV log. Temporary `[input]` log lines record keyboard
keycode/scancode, mouse button number and pointer position (motion is sampled
at most once every five seconds). After manually installing the new development
IPK, press OK once on a harmless menu row and inspect `uxplay.log` for the
resulting key and/or `mouse down button=1` lines. Check that a single press
opens only one page or changes only one setting, then verify arrows, Back and
Exit. Please share that short excerpt to confirm the final device mapping.

On the idle screen, use Up/Down to focus **Quality settings**, **Usage statistics**, **Live diagnostics** or **Capabilities** and press OK. Back returns to the home screen; Back from home exits when idle. Left/Right on home continues to adjust the existing audio delay. During playback, OK opens the diagnostic page and Back hides it; Back from the normal playback view disconnects as before. The sender PIN remains visible on the home screen.

Quality settings shows the active 720p/30 profile and explains why 1080p, 60 FPS and 4K are locked. The HDR policy can be set to Auto or SDR, but both use the current SDR pipeline; Auto does **not** indicate detected HDR. On-screen diagnostics can be Off, Compact or Detailed. The default is Off. Changes to these UI preferences are saved atomically in `state/settings-v1` and take effect without renegotiating an active session. Unsupported quality values in an old settings file fall back to 720p/30 at load time.

The Starfish/ACB decoded image may be on a plane above SDL. The optional playback overlay is deliberately small and refreshed once per second, but its visibility on Rockhopper 3.9.0 is **unverified**. The same metrics are available from the in-app diagnostics page while idle; it is the reliable fallback. If a B7 test shows that SDL is behind the video plane, leave the overlay Off and use the idle dashboard and `uxplay.log`. This phase does not force Starfish into a software-rendered path.

## Local usage history

`state/usage-v1` is a small, versioned text file written through a temporary file and rename. It stores aggregate playback time, successful session counts by mode, incoming and accepted-feed frame totals, packet/video frame drops, audio or hardware decoder error counts, session errors, and the latest 16 session summaries. Each summary has UTC start time, duration, mode, observed H.264 SPS resolution, and frame/error counts. It contains no sender name, IP address, pairing key or device identifier. The existing `paired.keys`, `device.id`, `server.key` and audio-delay file are untouched.

A successful session requires accepted video and a Starfish playing event. Playback time is the monotonic interval from media opening until the session closes; it is not display-on time. Incoming and feed averages divide counts by that duration and include static-image holds. Audio-only and direct-media counts remain zero until those paths actually play. A transport failure before successful playback increments the error total but not the successful-session total. The statistics page offers a two-step **Reset usage history** action; resetting it writes only `usage-v1` and preserves pairing.

## Live diagnostics semantics

- **Incoming resolution** comes from the H.264 SPS. The advertised display profile is 1280x720; portrait streams up to the same pixel area and long edge are accepted without stretching the encoded content. Unsupported input dimensions are rejected with a clear log/status instead of silently being called 720p.
- **Incoming FPS** counts access units at the receiver callback in a one-second wall-clock window. **Feed FPS** counts H.264 access units accepted by Starfish in that window. **PTS FPS** is the latest five-second source timestamp estimate.
- **Displayed FPS** is N/A. Firmware `FRAMEREADY` callbacks are throttled, and `Feed` acceptance does not prove presentation.
- Video/audio queue packet counts, video queue residence, Starfish `Feed` call time, frame/packet drops, output audio format, session uptime and process RSS are diagnostic values. Queue/feed latency fields use the latest five-second window; they are not decoder or panel latency.
- H.264 bit depth, HDR metadata, source audio sample rate and network throughput are not currently confirmed by the application; the UI says so instead of inferring values.

## B7 acceptance procedure for this phase

1. Keep the prior working development IPK and its checksum. Download and verify the new CI artifact before any manual installation. CI never contacts the TV.
2. With no sender connected, navigate every page using the Magic Remote. Change Auto/SDR and Off/Compact/Detailed, exit and relaunch, and verify that the choices persist. Check that 1080p, 60 FPS and 4K cannot be selected. Confirm that the pairing PIN remains visible when required.
3. Play the known-good 720p/30 mirroring sequence with photos, video and stereo audio. Observe the SPS resolution, incoming/feed rates and queue counts in the dashboard. If trying the overlay, note whether it actually appears above video and whether it affects playback; turn it Off if not. Do not interpret the firmware callback rate as displayed FPS.
4. Disconnect normally, relaunch, and check that one successful mirroring session, its UTC start/duration and averages were retained. Exercise the two-step usage-history reset and verify that pairing still works without a new PIN. Repeat a reconnect and one two-minute playback session.
5. Save `uxplay.log` and any failure timestamps. Restore the saved previous development IPK if navigation, pairing, media playback or stability regresses. The production app ID remains separate.

This phase's package has not been run on the B7 until the owner performs those steps. Experimental 1080p negotiation starts only after phase-one CI and build results have been reported.

## Phase 2: experimental 1080p/30 screen mirroring

The default remains 720p/30. On the idle Quality Settings page, select **1080p / 30 FPS** to advertise 1920×1080 to the next sender session. Select **720p / 30 FPS** for the known-good recovery profile. Left/Right on either quality row also switches between these two profiles. The receiver restarts its discovery service only while no sender or pairing exchange is active. The sender must disconnect and reconnect after a change. The choice is saved in `state/settings-v1` and restored at launch; pairing files and receiver identity are unchanged. A failed receiver startup restores the previous profile; a failed 1080p Starfish decoder initialization restores 720p for the next session. An oversized or unsupported incoming SPS ends only that session.

UxPlay's mirror plist receives the selected 1280×720 or 1920×1080 maximum and 30 FPS. The H.264 SPS remains the source of truth for **incoming** resolution. The diagnostics page and overlay show `Incoming SPS` separately from `Advertised`. A sender that continues to send 720p after selecting 1080p is reported as 720p; neither the UI nor statistics claim native 1080p in that case. Source width/height are passed to Starfish, with a centered aspect-preserving ACB window inside the selected canvas. Portrait input is allowed within the same long-edge and pixel-area budget. The 1080p profile increases the video packet/source-buffer ceiling from 1 to 2 MiB and the shared queue from 4 to 8 MiB; the 512-packet limit remains. The media pipeline, H.264 codec and 30 FPS setting are otherwise unchanged.

### LG B7 manual comparison

1. Keep the previously working development IPK and its SHA-256. Download the new CI artifact, verify `SHA256SUMS` and `ELF_REPORT.txt`, then install it manually with webOS Dev Manager. CI does not contact the TV.
2. Start at **720p / 30 FPS**. Reconnect the sender, mirror a static photo, advance through photos, and play two minutes of video with stereo audio. Confirm Magic Remote navigation, pairing, overlay, and diagnostics still work. Record `Advertised 1280 x 720` and actual SPS dimensions, FPS, drops, retries and RSS.
3. Disconnect the sender. Select **1080p / 30 FPS** using OK or Right. Verify the UI requests reconnection, exit and relaunch the app, and confirm the selection persisted. Reconnect the same sender and capture its reported geometry, actual SPS width/height and the Starfish Load log. A displayed 1920×1080 canvas with a 1280×720 SPS is **not** a successful native 1080p test.
4. Mirror landscape and portrait content, then repeat the photo transitions and two-minute video/audio playback. Compare input/PTS/feed FPS, queue occupancy, drops, feed retries, decoder errors, memory and visual stability against step 2. Confirm the picture is not stretched or clipped. If no 1920×1080 or 1080×1920 SPS arrives, record the sender model and log excerpt as a negotiation result, not as a B7 decoder failure.
5. Disconnect and select **720p / 30 FPS**, then reconnect. If 1080p cannot initialize, confirm automatic 720p recovery and a clear status message. If the app cannot launch, restore the saved previous development IPK in webOS Dev Manager. Do not uninstall the production app or erase private state.

60 FPS, 4K, HEVC, HDR and direct-media streaming remain gated. The new 1080p path is **unverified on the LG B7** until these hardware checks are completed.
