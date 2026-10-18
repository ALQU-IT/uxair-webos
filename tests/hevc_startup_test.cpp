#include "hevc_startup.h"
#include <cassert>

struct MockDecoder {
    HevcStartupGate gate;
    bool loaded=false, failed=false, playing=false;
    unsigned accepted=0;
    void load(int64_t now_ns=1000000000LL) {
        gate.accept_load(now_ns);loaded=false;failed=false;playing=false;accepted=0;
    }
    bool feed(size_t bytes) {
        HevcFeedStage stage=gate.video_stage(loaded,failed,bytes);
        if(stage!=HevcFeedStage::Preroll && stage!=HevcFeedStage::Loaded) return false;
        gate.accepted_video(bytes,stage);++accepted;
        return true;
    }
    void tick() { if(loaded && accepted) playing=true; } // Mock of Play acceptance.
};

int main() {
    MockDecoder decoder;
    assert(decoder.gate.video_stage(false,false,4096)==HevcFeedStage::NoLoad);
    decoder.load(); // Starfish Load returned success; callback is delayed.
    unsigned queue_peak=0, queued=0;
    for(unsigned frame=0;frame<30;++frame) {
        ++queued;
        if(decoder.feed(4096)) --queued;
        if(frame==4) decoder.loaded=true; // LOADCOMPLETED after input preroll.
        decoder.tick();
        if(queued>queue_peak)queue_peak=queued;
    }
    assert(decoder.accepted==30 && decoder.playing && queue_peak==0);
    assert(decoder.gate.preroll_frames()==5);
    assert(decoder.gate.accepted_frames()==30);

    decoder.load(); // Callback never arrives: feed must stop at a finite bound.
    for(unsigned frame=0;frame<240;++frame)decoder.feed(4096);
    assert(decoder.accepted==HevcStartupGate::kMaxPrerollFrames);
    assert(decoder.gate.video_stage(false,false,4096)==HevcFeedStage::WaitingForLoad);
    assert(!decoder.playing); // The receiver's eight-second deadline triggers H.264 recovery.
    decoder.loaded=true;
    assert(decoder.feed(4096)); // A late valid callback releases the gate.
    decoder.tick();assert(decoder.playing);

    decoder.load();
    assert(decoder.feed(HevcStartupGate::kMaxPrerollBytes));
    assert(decoder.gate.video_stage(false,false,1)==HevcFeedStage::WaitingForLoad);
    decoder.failed=true;
    assert(decoder.gate.video_stage(true,true,1)==HevcFeedStage::Failed);
    assert(!decoder.feed(1));

    decoder.gate.reset(); // Decoder reinitialization for portrait-to-UHD SPS change.
    assert(decoder.gate.video_stage(false,false,4096)==HevcFeedStage::NoLoad);
    decoder.load();assert(decoder.feed(4096));
    assert(decoder.gate.preroll_frames()==1);

    // Reported B7 case: the session has already fed 605 frames when a new
    // portrait-to-UHD decoder Load succeeds at session age 23.72 seconds.
    const int64_t session_start=1000000000LL;
    const int64_t reopen_at=session_start+23720000000LL;
    decoder.load(session_start);decoder.loaded=true;
    for(unsigned i=0;i<605;++i)assert(decoder.feed(4096));
    decoder.tick();assert(decoder.playing && decoder.gate.accepted_frames()==605);
    decoder.load(reopen_at);
    assert(decoder.gate.accepted_frames()==0 && decoder.gate.preroll_frames()==0);
    assert(!decoder.gate.startup_timed_out(reopen_at+200000000LL,false,false,false));
    bool retained_valid_keyframe=true; // main.cpp holds the validated pending AU.
    assert(retained_valid_keyframe && decoder.feed(4096));
    assert(decoder.gate.preroll_frames()==1 && decoder.gate.accepted_frames()==1);
    decoder.loaded=true;decoder.tick();
    assert(decoder.playing && !decoder.gate.startup_timed_out(reopen_at+9000000000LL,true,true,false));

    decoder.load(reopen_at); // Genuine failure still has a per-Load deadline.
    assert(!decoder.gate.startup_timed_out(reopen_at+HevcStartupGate::kStartupTimeoutNs,false,false,false));
    assert(decoder.gate.startup_timed_out(reopen_at+HevcStartupGate::kStartupTimeoutNs+1,false,false,false));
    assert(!decoder.gate.startup_timed_out(reopen_at+HevcStartupGate::kStartupTimeoutNs+1,false,false,true));
    decoder.loaded=true;
    assert(decoder.gate.startup_timed_out(reopen_at+HevcStartupGate::kStartupTimeoutNs+1,true,false,false));
    // H.264 startup does not consult the HEVC gate; its existing Feed path remains separate.
}
