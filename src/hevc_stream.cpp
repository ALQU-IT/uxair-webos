#include "hevc_stream.h"
#include "hevc_sample.h"
#include <algorithm>

namespace {
size_t start_code(const uint8_t *data, size_t size, size_t from, size_t& prefix) {
    for(size_t i=from; i+3<=size; ++i) {
        if(data[i] || data[i+1]) continue;
        if(data[i+2]==1) {prefix=3;return i;}
        if(i+4<=size && !data[i+2] && data[i+3]==1) {prefix=4;return i;}
    }
    return size;
}
}
void HevcStream::reset() {vps_.clear();sps_.clear();pps_.clear();width_=height_=0;vps_id_=sps_vps_id_=sps_id_=pps_sps_id_=-1;}
HevcFrameInfo HevcStream::inspect(const uint8_t *data, size_t size) {
    HevcFrameInfo result;
    if(!data || size<6 || size>hevc_probe::kMaxAccessUnit) return result;
    size_t prefix=0, pos=start_code(data,size,0,prefix);
    if(pos!=0) return result;
    bool vcl=false, changed=false;
    std::vector<uint8_t> vps=vps_,sps=sps_,pps=pps_;
    int width=width_,height=height_;
    int vps_id=vps_id_,sps_vps_id=sps_vps_id_,sps_id=sps_id_,pps_sps_id=pps_sps_id_;
    unsigned count=0;
    while(pos<size && ++count<=128) {
        size_t begin=pos+prefix, next_prefix=0;
        size_t end=start_code(data,size,begin,next_prefix);
        if(end-begin<2) return HevcFrameInfo{};
        const int type=(data[begin]>>1)&63;
        if((data[begin]&0x80) || !(data[begin+1]&7)) return HevcFrameInfo{};
        if(type==35) result.has_aud=true;
        if(type==32 || type==33 || type==34) {
            if(end-begin>65536) return HevcFrameInfo{};
            std::vector<uint8_t> nal(data+begin,data+end);
            if(type==33) {
                hevc_probe::Sample parsed;
                if(!hevc_probe::parse_sps(nal.data(),nal.size(),parsed) ||
                   parsed.width>3840 || parsed.height>2160 ||
                   int64_t(parsed.width)*parsed.height>3840LL*2160) return HevcFrameInfo{};
                width=parsed.width;height=parsed.height;
                sps_vps_id=parsed.vps_id;sps_id=parsed.sps_id;
                if(sps!=nal) {sps.swap(nal);changed=true;}
            } else if(type==32) {
                if(nal.size()<3) return HevcFrameInfo{};
                vps_id=nal[2]>>4;
                if(vps!=nal) {vps.swap(nal);changed=true;}
            } else {
                int pps_id;
                if(!hevc_probe::parse_pps_ids(nal.data(),nal.size(),pps_id,pps_sps_id)) return HevcFrameInfo{};
                if(pps!=nal) {pps.swap(nal);changed=true;}
            }
        } else if(type<=31) {
            vcl=true;
            if(type>=16 && type<=21) result.keyframe=true;
        }
        if(end==size) break;
        pos=end;prefix=next_prefix;
    }
    if(count>128 || !vcl || vps.empty() || sps.empty() || pps.empty() || !width ||
       vps_id<0 || sps_vps_id!=vps_id || sps_id<0 || pps_sps_id!=sps_id) return HevcFrameInfo{};
    vps_.swap(vps);sps_.swap(sps);pps_.swap(pps);
    width_=width;height_=height;
    vps_id_=vps_id;sps_vps_id_=sps_vps_id;sps_id_=sps_id;pps_sps_id_=pps_sps_id;
    result.valid=true;result.configuration_changed=changed;
    result.width=width_;result.height=height_;result.profile=1;result.bit_depth=8;
    return result;
}
bool ensure_hevc_aud(std::vector<uint8_t>& access_unit, bool has_aud) {
    if(has_aud) return !access_unit.empty() && access_unit.size()<=hevc_probe::kMaxAccessUnit;
    static const uint8_t aud[]={0,0,0,1,0x46,0x01,0x50};
    if(access_unit.empty() || access_unit.size()>hevc_probe::kMaxAccessUnit-sizeof(aud)) return false;
    access_unit.insert(access_unit.begin(),aud,aud+sizeof(aud));
    return true;
}
