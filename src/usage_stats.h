#pragma once
#include <cstdint>
#include <deque>
#include <string>

enum class SessionKind { Mirror = 1, Audio = 2, Direct = 3 };
struct SessionSummary {
    uint64_t started_utc = 0;
    uint64_t duration_ms = 0;
    SessionKind kind = SessionKind::Mirror;
    unsigned width = 0, height = 0;
    unsigned codec = 0; // 1 = H.264; 0 = unknown
    uint64_t incoming_frames = 0, feed_frames = 0;
    uint64_t packet_drops = 0, video_frame_drops = 0, decoder_errors = 0, session_errors = 0;
};
struct UsageStats {
    uint64_t playback_ms = 0, successful_sessions = 0;
    uint64_t mirror_sessions = 0, audio_sessions = 0, direct_sessions = 0;
    uint64_t incoming_frames = 0, feed_frames = 0;
    uint64_t packet_drops = 0, video_frame_drops = 0, decoder_errors = 0, session_errors = 0;
    std::deque<SessionSummary> recent; // at most 16, newest first
    void record(const SessionSummary& session);
    void reset();
};
bool load_usage_stats(const std::string& path, UsageStats& stats);
bool save_usage_stats(const std::string& path, const UsageStats& stats);
const char *session_kind_name(SessionKind kind);
