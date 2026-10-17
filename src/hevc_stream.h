#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

struct HevcFrameInfo {
    bool valid=false, keyframe=false, configuration_changed=false, has_aud=false;
    int width=0, height=0, profile=0, bit_depth=0;
};

// UxPlay delivers one Annex B access unit per mirror callback. Parameter sets
// may be prepended to a random-access unit; cache them across subsequent AUs.
class HevcStream {
public:
    HevcFrameInfo inspect(const uint8_t *data, size_t size);
    void reset();
private:
    std::vector<uint8_t> vps_, sps_, pps_;
    int width_=0, height_=0;
    int vps_id_=-1, sps_vps_id_=-1, sps_id_=-1, pps_sps_id_=-1;
};
bool ensure_hevc_aud(std::vector<uint8_t>& access_unit, bool has_aud);
