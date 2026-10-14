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
