#pragma once
#include "settings.h"
#include <cstdint>

struct AdvertisedCapabilities {
    int width, height, max_fps;
    bool direct_video, hls, hevc, legacy_pairing;
};
inline AdvertisedCapabilities advertised_capabilities(const Settings& settings) {
    Settings safe=settings;validate_settings(safe);
    AdvertisedCapabilities result={safe.width,safe.height,safe.max_fps,false,false,false,true};
    return result;
}
// Pinned UxPlay 1.74 defaults from lib/dnssdint.h; these four overrides leave
// the effective DNS-SD mask unchanged. The mask alone does not prove playback.
static const uint64_t UXPLAY_PINNED_FEATURES = 0x000000005A7FFEE6ULL;
