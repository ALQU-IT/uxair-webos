#include "audio_session.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>

int main() {
    AirPlayAudioIntent intent;
    assert(!standalone_audio_candidate(intent,false));
    intent.configured=true;
    assert(!standalone_audio_candidate(intent,false)); // no negotiated codec
    for(unsigned codec : {1u,2u,4u,8u}) {
        intent.codec=codec;
        intent.using_screen=false;
        assert(standalone_audio_candidate(intent,false));
        assert(!standalone_audio_candidate(intent,true));
        intent.using_screen=true;
        assert(!standalone_audio_candidate(intent,false)); // preserve mirroring A/V lifecycle
    }
    intent.using_screen=false;
    for(unsigned codec : {3u,5u,6u,7u,9u}) {
        intent.codec=codec;
        assert(!standalone_audio_candidate(intent,false)); // unsupported RAOP packet formats
    }
    std::puts("PASS: standalone RAOP audio is gated by negotiated codec, usingScreen, and mirror state");
}
