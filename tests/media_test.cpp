#include "h264.h"
#include "audio.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iterator>
#include <random>
static std::vector<uint8_t> file(const char *name) {
    std::ifstream f(name,std::ios::binary);
    assert(f.good());return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),{});
}
int main(int argc,char **argv) {
    assert(argc==4);
    auto landscape=file(argv[1]);auto portrait=file(argv[2]);
    auto a=inspect_h264(landscape.data(),landscape.size());
    assert(a.valid&&a.keyframe&&a.picture&&a.width==1280&&a.height==720&&!a.configuration.empty());
    auto b=inspect_h264(portrait.data(),portrait.size());
    assert(b.valid&&b.width==720&&b.height==1280);
    assert(!inspect_h264(nullptr,0).valid);
    uint8_t broken[]={0,0,0,1,0x67,0xff};assert(!inspect_h264(broken,sizeof(broken)).valid);
    // Truncated SPS and malformed network data must never read outside the packet.
    for(size_t n=0;n<64;++n) inspect_h264(landscape.data(),n);
    std::mt19937 rng(12345);
    for(int i=0;i<10000;++i) {
        std::vector<uint8_t> noise(rng()%512);
        for(auto& c:noise)c=rng();
        if(noise.size()>5){noise[0]=noise[1]=noise[2]=0;noise[3]=1;noise[4]=0x67;}
        inspect_h264(noise.data(),noise.size());
    }
    AudioDecoder decoder;std::vector<uint8_t> pcm;
    uint8_t raw[]={0,0,0xff,0x7f,0,0x80,0,0};
    assert(decoder.decode(1,raw,sizeof(raw),pcm)&&pcm==std::vector<uint8_t>(raw,raw+sizeof(raw)));
    assert(!decoder.decode(1,raw,3,pcm));
    assert(!decoder.decode(255,raw,sizeof(raw),pcm));
    auto adts=file(argv[3]);size_t pos=0,total=0,packets=0;
    while(pos+7<=adts.size()) {
        const uint8_t *p=adts.data()+pos;
        size_t len=((p[3]&3)<<11)|(p[4]<<3)|(p[5]>>5);
        size_t hdr=p[1]&1?7:9;assert(len>hdr&&pos+len<=adts.size());
        assert(decoder.decode(4,p+hdr,len-hdr,pcm));
        assert(pcm.size()%4==0);total+=pcm.size()/4;++packets;pos+=len;
    }
    assert(total>=44100&&total<47000&&packets>40);
    decoder.reset();
    printf("PASS: H.264 landscape/portrait, malformed/truncated input (10k cases), PCM validation, AAC-LC -> stereo PCM (%zu samples)\n",total);
}
