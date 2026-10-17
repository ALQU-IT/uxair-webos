#pragma once

namespace hevc_probe {
// An in-memory gate for one process invocation. No on-disk pass flag is read.
class CurrentLaunchUhdGate {
public:
    void record_1080p_control_result(int result, bool stopped) {
        complete_ = result == 0 && !stopped;
    }
    bool permits_uhd() const { return complete_; }
private:
    bool complete_ = false;
};
}
