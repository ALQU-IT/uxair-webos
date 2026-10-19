#include "media_rate.h"
#include <cassert>
#include <cstdio>

int main() {
    MediaRateFields rate={0,0,0};
    assert(media_rate_fields(30,rate));
    assert(rate.video_fps_value==30 && rate.video_fps_scale==1 && rate.max_frame_rate==30);
    assert(media_rate_fields(60,rate));
    assert(rate.video_fps_value==60 && rate.video_fps_scale==1 && rate.max_frame_rate==60);
    assert(!media_rate_fields(24,rate));
    std::puts("PASS: Starfish frame-rate fields match the validated advertised maximum");
}
