# Milestone 2: LG OLED B7 frame pacing investigation

## Baseline from the tested development IPK

The owner tested a 140-second mirroring session on an LG OLED B7 with Rockhopper 3.9.0. AirPlay discovery, mirroring, photos, video and stereo audio worked at H.264 1280x720/30. Continuous playback had incoming and Starfish feed rates near 30 FPS, zero packet drops and zero video/audio feed retries, zero audio decode errors, and about 33–37 MB process RSS. The mixed audio/video queue peaked at 262 packets. A video PTS interval reached about 1866.67 ms. Minor stutter was most visible between photos and also occurs in the upstream application. These observations do not identify the stage responsible for the stutter.

## Code findings and limits

- The AirPlay callback supplies video access units with remote PTS. The receiver copies them into one bounded audio/video deque. Previously that payload copy ran under the shared queue mutex. It now runs before locking, so copying a video access unit cannot directly hold up an audio callback. The queue still selects the lowest PTS by scanning the deque; its cost is measured before changing this ordering rule.
- The main loop selects only packets whose PTS is due, then calls Starfish `Feed` synchronously. It deliberately gives a new session 200 ms of pre-roll. This pre-roll is now anchored after synchronous decoder initialization; the earlier code sampled the clock before `Load`, which could consume some or all of the intended pre-roll. The initial 200 ms value and the 720p/30 configuration are unchanged.
- A large PTS interval can result from a sender holding a static photo, irregular delivery, or a sender timestamp jump. Neither PTS nor average FPS alone distinguishes those cases. The new counters pair arrival and PTS gaps on the same access unit, and report video-only queue occupancy, queue residence, dequeue-to-feed delay, scheduling lateness, main-loop gaps, parse time, and synchronous feed time.
- `FRAMEREADY` is throttled by the TV and cannot measure hardware frame cadence. Starfish feed acceptance is not presentation. Decoder latency remains unproven without a firmware frame timestamp or external camera measurement.

## Reading the new log lines

`[pacing]` reports maximum wall-clock inter-arrival, remote PTS, and accepted-feed gaps in the five-second window. `both`, `arrival_only`, and `pts_only` count gaps greater than 100 ms on the same incoming access unit. `video_idle_ms` is time since the last video callback. `mirror` and `paused` provide session context. `callbacks_idle_ms` is only the age of the last throttled `FRAMEREADY` event.

`[queue-video]` separates video packet and byte occupancy from the existing mixed queue. `ingress_to_queue` includes the callback copy and lock wait. `queue_residence` is enqueue to dequeue; it can include intentional waiting for PTS. `pending_to_feed` is dequeue to the start of an accepted `Feed`, including any initial pre-roll. `schedule_late` is the positive difference between feed start and the scheduled PTS. Durations are average/maximum milliseconds within the window unless shown as a single maximum. The peak counts remain cumulative for the process.

`[stage]` reports the maximum main-loop interval, media tick, and synchronous audio/video `Feed` durations, counts over 50 ms, and audio queue residence. These distinguish a stalled receiver thread from a sparse sender where possible. The existing `[video]`, `[pipeline]`, `[audio]`, and `[health]` lines remain available.

For a photo held static by the sender, a large arrival gap with no video queued, a similarly large PTS gap, and no long loop/feed event is consistent with an intentional hold. The receiver has no semantic signal that says whether a photo was intentionally held; label photo-change times in the test notes. If arrivals continue but queue residence, pending-to-feed, or loop gaps rise, investigate receiver scheduling. If `Feed` time/retries rise while arrivals remain regular, investigate the media pipeline. None of these counters alone proves time to display on the panel.

## GStreamer warnings during initialization

The B7 log showed `gst_caps_get_structure: assertion 'GST_IS_CAPS (caps)' failed`, followed by null structure/value/unref assertions after Starfish announced `video/x-h264` and `audio/x-raw`, and before `[session]` reported 1280x720. The application itself does not call GStreamer caps APIs; it passes a BUFFERSTREAM JSON payload to Starfish and ACB. `Load` and later playback succeeded, so the warning is not proof of a rejected format. The exact origin is unresolved: a parameter interpreted unexpectedly by the proprietary APIs, an ACB/Starfish call sequence issue, or a firmware internal null-caps path are all possible. In particular, the logged PCM `sampleRate` is 44.1 (kHz) and firmware's audio event echoes 44.099998; this is not evidence that the field should be 44100. The API spelling `seperatedPTS` is used in the published Starfish sample and should not be changed on speculation.

`[media-init]` now brackets ACB creation/initialization, Starfish construction/Load, media ID registration, display window setup, video-info forwarding, Play, and ACB state changes. On the next owner-run session, the lines immediately surrounding the warnings should narrow which call or asynchronous callback emits them. They remain visible. No Starfish/ACB initialization parameters, decoder backend, or playback order were changed.

## Before/after procedure on the LG B7

1. Save the known-good development IPK and its log. Keep the production app and its pairing state untouched. Install the new **development** IPK only when explicitly choosing to test it; CI never connects to the TV.
2. With the same sender, Wi-Fi, and 720p/30 setting, start mirroring. Hold a photo still for ten seconds, advance through ten photos at a steady pace, then play two minutes of continuous motion with stereo audio. Note wall-clock times of each tap and every visible hitch. Repeat three times on each build.
3. Capture `uxplay.log` and the GStreamer warning context. Compare `[pacing]` arrival/PTS/feed gaps, `[queue-video]` video queue depth and residence, `[stage]` loop/feed maxima, `[pipeline]` drops/retries, `[audio]` errors, and `[health]` RSS/session resets around each noted transition. Preserve full five-second windows and compare like phases, not averages across a static hold.
4. Record the sender and TV together with a high-frame-rate camera for at least one photo sequence and one moving clip. Count frame intervals at the panel; accepted `Feed` is not a displayed-frame timestamp.
5. Verify discovery, PIN pairing/reconnect, mirroring, photos, video, stereo audio, and cleanup. If a regression appears, close the dev app and reinstall the saved known-good **development** IPK; the production app remains a separate rollback path.

The owner subsequently tested Milestone 2 on the B7 and reported stable functionality with minor video judder comparable to the original receiver. The instrumentation cannot by itself distinguish intentional still frames from sender-side stalls or quantify panel decode latency.
