#pragma once
#include <cstddef>

// Load() acceptance permits a bounded input preroll. It does not imply that
// Starfish has emitted LOADCOMPLETED or that decoded video is being displayed.
enum class HevcFeedStage { NoLoad, Failed, Preroll, WaitingForLoad, Loaded };
class HevcStartupGate {
public:
    static const unsigned kMaxPrerollFrames = 60;
    static const size_t kMaxPrerollBytes = 8u * 1024u * 1024u;

    void reset() { load_accepted_=false; preroll_frames_=0; preroll_bytes_=0; }
    void accept_load() { reset(); load_accepted_=true; }
    HevcFeedStage video_stage(bool load_completed, bool failed, size_t bytes) const {
        if(failed) return HevcFeedStage::Failed;
        if(!load_accepted_) return HevcFeedStage::NoLoad;
        if(load_completed) return HevcFeedStage::Loaded;
        if(!bytes || preroll_bytes_>=kMaxPrerollBytes ||
           bytes>kMaxPrerollBytes-preroll_bytes_ || preroll_frames_>=kMaxPrerollFrames)
            return HevcFeedStage::WaitingForLoad;
        return HevcFeedStage::Preroll;
    }
    void accepted_video(size_t bytes, HevcFeedStage stage) {
        if(stage==HevcFeedStage::Preroll) { ++preroll_frames_; preroll_bytes_+=bytes; }
    }
    unsigned preroll_frames() const { return preroll_frames_; }
    size_t preroll_bytes() const { return preroll_bytes_; }
private:
    bool load_accepted_=false;
    unsigned preroll_frames_=0;
    size_t preroll_bytes_=0;
};
