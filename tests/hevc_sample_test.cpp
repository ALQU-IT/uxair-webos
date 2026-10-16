#include "hevc_sample.h"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

static std::vector<uint8_t> read(const char *path) {
    std::ifstream f(path,std::ios::binary);
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>());
}
int main(int argc,char **argv) {
    assert(argc==3);
    const int widths[]={1920,3840},heights[]={1080,2160};
    for (int i=0;i<2;++i) {
        auto data=read(argv[i+1]);
        assert(!data.empty());
        hevc_probe::Sample parsed; std::string error;
        assert(hevc_probe::inspect(data.data(),data.size(),widths[i],heights[i],parsed,error));
        assert(parsed.width==widths[i] && parsed.height==heights[i]);
        assert(parsed.profile==1 && parsed.bit_depth==8 && parsed.units.size()==30);
        assert(!hevc_probe::inspect(data.data(),data.size(),widths[1-i],heights[1-i],parsed,error));
        data[0]=0xff;
        assert(!hevc_probe::inspect(data.data(),data.size(),widths[i],heights[i],parsed,error));
        data=read(argv[i+1]);
        assert(!hevc_probe::inspect(data.data(),15,widths[i],heights[i],parsed,error));
        assert(!hevc_probe::inspect(data.data(),hevc_probe::kMaxFile+1,widths[i],heights[i],parsed,error));
        assert(!hevc_probe::inspect(nullptr,data.size(),widths[i],heights[i],parsed,error));
        // Truncating an access unit must fail, never read beyond its buffer.
        assert(!hevc_probe::inspect(data.data(),3,widths[i],heights[i],parsed,error));
    }
}
