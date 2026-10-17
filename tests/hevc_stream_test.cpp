#include "hevc_stream.h"
#include "hevc_sample.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <vector>

static std::vector<uint8_t> read(const char *path) {
    std::ifstream f(path,std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
}
int main(int argc,char **argv) {
    assert(argc==4);
    HevcStream stream;
    for(int test=0;test<2;++test) {
        std::vector<uint8_t> bytes=read(argv[test+1]);
        hevc_probe::Sample sample;std::string error;
        const int width=test?3840:1920,height=test?2160:1080;
        assert(hevc_probe::inspect(bytes.data(),bytes.size(),width,height,sample,error));
        assert(sample.units.size()==30);
        for(size_t i=0;i<sample.units.size();++i) {
            const auto& unit=sample.units[i];
            HevcFrameInfo frame=stream.inspect(bytes.data()+unit.offset,unit.size);
            assert(frame.valid && frame.width==width && frame.height==height);
            assert(frame.profile==1 && frame.bit_depth==8);
            if(i==0) assert(frame.has_aud);
            if(i==0) assert(frame.keyframe && frame.configuration_changed);
        }
    }
    stream.reset();
    std::vector<uint8_t> portrait=read(argv[3]);
    assert(!portrait.empty());
    HevcFrameInfo portrait_frame=stream.inspect(portrait.data(),portrait.size());
    assert(portrait_frame.valid && portrait_frame.keyframe && portrait_frame.configuration_changed);
    assert(portrait_frame.width==996 && portrait_frame.height==2160);
    std::vector<uint8_t> uhd=read(argv[2]);
    hevc_probe::Sample uhd_sample;std::string uhd_error;
    assert(hevc_probe::inspect(uhd.data(),uhd.size(),3840,2160,uhd_sample,uhd_error));
    const auto& uhd_first=uhd_sample.units[0];
    HevcFrameInfo changed=stream.inspect(uhd.data()+uhd_first.offset,uhd_first.size);
    assert(changed.valid && changed.keyframe && changed.configuration_changed);
    assert(changed.width==3840 && changed.height==2160);
    stream.reset();
    std::vector<uint8_t> bytes=read(argv[1]);
    hevc_probe::Sample sample;std::string error;
    assert(hevc_probe::inspect(bytes.data(),bytes.size(),1920,1080,sample,error));
    const auto& second=sample.units[1];
    assert(!stream.inspect(bytes.data()+second.offset,second.size).valid); // no VPS/SPS/PPS after reconnect
    const auto& first=sample.units[0];
    assert(!stream.inspect(bytes.data()+first.offset,hevc_probe::kMaxAccessUnit+1).valid);
    assert(!stream.inspect(nullptr,first.size).valid);
    std::vector<uint8_t> malformed(bytes.begin()+first.offset,bytes.begin()+first.offset+first.size);
    malformed[0]=0xff;
    assert(!stream.inspect(malformed.data(),malformed.size()).valid);
    malformed.assign(bytes.begin()+first.offset,bytes.begin()+first.offset+first.size);
    malformed[5]=0x80;
    assert(!stream.inspect(malformed.data(),malformed.size()).valid);
    std::vector<uint8_t> no_aud={0,0,0,1,0x26,0x01,0x80};
    assert(ensure_hevc_aud(no_aud,false));
    assert(no_aud.size()==14 && no_aud[4]==0x46);
    assert(ensure_hevc_aud(no_aud,true) && no_aud.size()==14);
    std::vector<uint8_t> too_big(hevc_probe::kMaxAccessUnit,0);
    assert(!ensure_hevc_aud(too_big,false));
}
