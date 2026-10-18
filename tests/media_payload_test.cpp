#include "media_payload.h"
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    MediaRateFields rate30;
    assert(media_rate_fields(30,rate30));
    const std::string baseline_video=starfish_load_payload(
        "com.github.depius.uxair","H264",1920,1080,true,44100,1048576,rate30,false);
    const std::string expected_baseline_video=
        "{\"args\":[{\"mediaTransportType\":\"BUFFERSTREAM\",\"option\":{\"appId\":\"com.github.depius.uxair\",\"queryPosition\":false,\"externalStreamingInfo\":{\"contents\":{\"codec\":{\"video\":\"H264\",\"audio\":\"PCM\"},\"esInfo\":{\"pauseAtDecodeTime\":true,\"ptsToDecode\":0,\"seperatedPTS\":true,\"videoFpsValue\":30,\"videoFpsScale\":1},\"format\":\"RAW\",\"provider\":\"Chrome\",\"pcmInfo\":{\"channelMode\":\"stereo\",\"format\":\"S16LE\",\"sampleRate\":44.1,\"bitsPerSample\":16,\"layout\":\"interleaved\"}},\"restartStreaming\":false,\"streamQualityInfo\":true,\"streamQualityInfoNonFlushable\":true,\"totalStreamSize\":256,\"bufferingCtrInfo\":{\"preBufferByte\":0,\"bufferMinLevel\":0,\"bufferMaxLevel\":0,\"qBufferLevelAudio\":0,\"qBufferLevelVideo\":0,\"srcBufferLevelAudio\":{\"minimum\":1024,\"maximum\":131072},\"srcBufferLevelVideo\":{\"minimum\":1024,\"maximum\":1048576}}},\"adaptiveStreaming\":{\"audioOnly\":false,\"maxWidth\":1920,\"maxHeight\":1080,\"maxFrameRate\":30}}}]}";
    assert(baseline_video==expected_baseline_video);

    MediaRateFields rate;
    assert(media_rate_fields(60,rate));
    const std::string audio=starfish_load_payload("com.github.depius.uxair","H264",0,0,true,44100,1048576,rate,true);
    assert(audio.find("\"codec\":{\"audio\":\"PCM\"}")!=std::string::npos);
    assert(audio.find("\"pcmInfo\":{\"channelMode\":\"stereo\"")!=std::string::npos);
    assert(audio.find("\"sampleRate\":44.1")!=std::string::npos);
    assert(audio.find("\"srcBufferLevelAudio\"")!=std::string::npos);
    assert(audio.find("\"video\"")==std::string::npos);
    assert(audio.find("\"videoFpsValue\"")==std::string::npos);
    assert(audio.find("\"adaptiveStreaming\"")==std::string::npos);\n    assert(audio.size()>=4 && audio.substr(audio.size()-4)=="}}]}");

    const std::string video=starfish_load_payload("com.github.depius.uxair","H265",3840,2160,true,44100,2097152,rate,false);
    assert(video.find("\"codec\":{\"video\":\"H265\",\"audio\":\"PCM\"}")!=std::string::npos);
    assert(video.find("\"videoFpsValue\":60")!=std::string::npos);
    assert(video.find("\"adaptiveStreaming\":{\"audioOnly\":false,\"maxWidth\":3840,\"maxHeight\":2160,\"maxFrameRate\":60}")!=std::string::npos);
    std::puts("PASS: legacy H264/30 mirror payload is byte-equivalent; audio-only and HEVC/60 payloads retain expected fields");
}
