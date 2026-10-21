# Local usage history

UxAir stores usage statistics in the app's private `state/usage-v1` file. The versioned format migrates prior `UXPLAY_USAGE_V1` lifetime counters and up to 16 summaries into the bounded `UXAIR_USAGE_V2` format on the next save. Migration does not invent old daily usage; chart coverage begins at the upgrade date.

The file is limited to 64 KiB, the recent-session list to 100 completed sessions, and daily aggregates to a rolling 365 days. Session summaries contain UTC start time, duration, mode, incoming dimensions/codec and receiver counters. They do not contain sender names, addresses, media titles, PINs or credentials. Pairing keys, receiver identity, video settings and logs are separate and are never reset with History.

History shows lifetime session/playback totals, last-30-day mirroring and music durations, a 30-day daily chart, and a 12-month monthly chart. Daily bars use UTC dates. Dates before the stored coverage date are unavailable, not zero-use. A monthly bar is marked with `*` if it is the current month or saved coverage starts partway through that month. Playback distribution percentages use last-30-day duration and round to a sum of 100 when there is data.

An unreadable or oversized history file is preserved and is not overwritten by normal saves. The owner can explicitly confirm **Reset history** to replace it with an empty history. A session already playing is not stopped by reset; if it later completes successfully, it is recorded as a new post-reset session.
