# Local usage history

UxAir stores usage statistics only in the development app’s `state/usage-v1` file. The format header is versioned; on first read, the prior `UXPLAY_USAGE_V1` lifetime counters and up to 16 existing summaries are retained and rewritten in the bounded `UXAIR_USAGE_V2` format on the next save. The migration does not invent old daily usage: daily charts begin at the upgrade date.

The file is limited to 64 KiB, the recent-session list to 100 completed sessions, and daily aggregates to a rolling 365 entries. Session summaries contain UTC start time, duration, mode, incoming dimensions/codec and receiver counters. They do not contain Apple device names, sender addresses, media titles, PINs or credentials. Only this usage file and its atomic-write temporary file count toward the History storage size. Pairing keys, receiver identity, settings and logs are separate and are never reset with History.

An unreadable or oversized history file is preserved and not overwritten by normal saves. The owner can explicitly confirm **Reset history** to replace it with an empty history. A session already playing is not stopped by reset; if it later completes successfully, it is recorded as a new post-reset session.

Daily bars use UTC dates. A day before the migration coverage date is shown as unavailable rather than zero; dates with recorded coverage and no playback are shown as zero use. The 12-month chart reports monthly aggregates. Usage periods summarize receiver playback records, not all activity on the TV.
