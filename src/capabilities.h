#pragma once
#include "settings.h"
#include <cstdint>

// The public Starfish API exposes setHdrInfo(), but this build has no verified
// Rockhopper 3.9 payload contract or output-plane confirmation for HDR10.
static const bool UXPLAY_HDR10_EXPERIMENTAL_ENABLED = false;

struct AdvertisedCapabilities {
    int width, height, max_fps;
    bool direct_video, hls, hevc, legacy_pairing;
};
inline AdvertisedCapabilities advertised_capabilities(const Settings& settings) {
    Settings safe=settings;validate_settings(safe);
    AdvertisedCapabilities result={safe.width,safe.height,safe.max_fps,false,false,
        safe.codec==MirrorCodec::ExperimentalHevc,true};
    return result;
}
// Pinned UxPlay 1.74 defaults from lib/dnssdint.h; these four overrides leave
// the effective DNS-SD mask unchanged. The mask alone does not prove playback.
static const uint64_t UXPLAY_PINNED_FEATURES = 0x000000005A7FFEE6ULL;
