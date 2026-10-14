#pragma once
#include <algorithm>
#include <cstdint>

struct VideoTimingSample {
    uint64_t frames = 0;
    uint64_t intervals = 0;
    uint64_t interval_total_ns = 0;
    uint64_t interval_min_ns = 0;
    uint64_t interval_max_ns = 0;
};

// Tracks incoming access-unit timestamps separately from throttled decoder events.
class VideoTiming {
public:
    void record(uint64_t pts_ns) {
        ++window_.frames;
        if (have_previous_ && pts_ns > previous_pts_) {
            const uint64_t interval = pts_ns - previous_pts_;
            // Clock jumps are handled by the session loop, not frame cadence.
            if (interval <= 2000000000ULL) {
                if (!window_.intervals) window_.interval_min_ns = interval;
                window_.interval_min_ns = std::min(window_.interval_min_ns, interval);
                window_.interval_max_ns = std::max(window_.interval_max_ns, interval);
                window_.interval_total_ns += interval;
                ++window_.intervals;
            }
        }
        previous_pts_ = pts_ns;
        have_previous_ = true;
    }
    VideoTimingSample take_window() {
        VideoTimingSample sample = window_;
        window_ = VideoTimingSample();
        return sample;
    }
    void reset() {
        window_ = VideoTimingSample();
        previous_pts_ = 0;
        have_previous_ = false;
    }
private:
    VideoTimingSample window_;
    uint64_t previous_pts_ = 0;
    bool have_previous_ = false;
};
