#include "media_payload.h"
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    MediaRateFields rate;
    assert(media_rate_fields(60,rate));
    const std::string audio=starfish_load_payload("com.github.depius.uxair","H264",0,0,true,44100,1048576,rate,true);
    assert(audio.find("\"codec\":{\"audio\":\"PCM\"}")!=std::string::npos);
    assert(audio.find("\"pcmInfo\":{\"channelMode\":\"stereo\"")!=std::string::npos);
    assert(audio.find("\"sampleRate\":44.1")!=std::string::npos);
    assert(audio.find("\"srcBufferLevelAudio\"")!=std::string::npos);
    assert(audio.find("\"video\"")==std::string::npos);
    assert(audio.find("\"videoFpsValue\"")==std::string::npos);
    assert(audio.find("\"adaptiveStreaming\"")==std::string::npos);

    const std::string video=starfish_load_payload("com.github.depius.uxair","H265",3840,2160,true,44100,2097152,rate,false);
    assert(video.find("\"codec\":{\"video\":\"H265\",\"audio\":\"PCM\"}")!=std::string::npos);
    assert(video.find("\"videoFpsValue\":60")!=std::string::npos);
    assert(video.find("\"adaptiveStreaming\":{\"audioOnly\":false,\"maxWidth\":3840,\"maxHeight\":2160,\"maxFrameRate\":60}")!=std::string::npos);
    std::puts("PASS: audio-only Starfish payload omits video configuration and mirror payload retains its codec and rate");
}
