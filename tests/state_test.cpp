#include "settings.h"
#include "capabilities.h"
#include "usage_stats.h"
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <unistd.h>

int main() {
    char directory[]="/tmp/uxplay-state-test-XXXXXX";
    assert(mkdtemp(directory));
    const std::string root(directory),settings_path=root+"/settings-v1",usage_path=root+"/usage-v1";
    const std::string keys_path=root+"/paired.keys";
    { std::ofstream keys(keys_path.c_str());keys<<"pairing-data-must-survive\n"; }

    Settings settings;
    assert(supported_mirror_resolution(1280,720));
    assert(supported_mirror_resolution(1920,1080));
    assert(supported_mirror_resolution(3840,2160));
    assert(supported_mirror_fps(30) && !supported_mirror_fps(60));
    assert(input_fits_mirror_profile(settings,1280,720));
    assert(input_fits_mirror_profile(settings,720,1280)); // portrait orientation
    assert(!input_fits_mirror_profile(settings,1920,1080));
    assert(!input_fits_mirror_profile(settings,1080,1920));
    assert(!set_mirror_resolution(settings,3840,2160));
    assert(settings.codec==MirrorCodec::H264);
    assert(!set_mirror_fps(settings,60));
    assert(settings.width==1280 && settings.height==720 && settings.max_fps==30);
    AdvertisedCapabilities profile=advertised_capabilities(settings);
    assert(profile.width==1280 && profile.height==720 && profile.max_fps==30);
    assert(mirror_limits(settings).video_packet_bytes==1024*1024);
    assert(mirror_limits(settings).queue_bytes==4*1024*1024);
    assert(!profile.direct_video && !profile.hls && !profile.hevc && profile.legacy_pairing);
    assert((UXPLAY_PINNED_FEATURES & (1ULL<<0))==0 && (UXPLAY_PINNED_FEATURES & (1ULL<<4))==0);
    assert((UXPLAY_PINNED_FEATURES & (1ULL<<42))==0 && (UXPLAY_PINNED_FEATURES & (1ULL<<27))!=0);
    settings.hdr=HdrPolicy::Sdr;settings.diagnostics=DiagnosticsMode::Detailed;
    assert(save_settings(settings_path,settings));
    Settings loaded;
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1280 && loaded.height==720 && loaded.max_fps==30);
    assert(loaded.hdr==HdrPolicy::Sdr && loaded.diagnostics==DiagnosticsMode::Detailed);
    int attempts=0;
    assert(switch_mirror_profile(settings,1920,1080,[&](const Settings& s){
        ++attempts;return s.width==1280;
    })==ProfileSwitchResult::Recovered);
    assert(attempts==2 && settings.width==1280); // failed activation rolls back
    assert(switch_mirror_profile(settings,1920,1080,[&](const Settings&){return false;})==ProfileSwitchResult::Unavailable);
    assert(settings.width==1280 && settings.height==720);
    assert(switch_mirror_profile(settings,3840,2160,[&](const Settings&){++attempts;return true;})==ProfileSwitchResult::Unchanged);
    assert(attempts==2);
    assert(switch_mirror_profile(settings,1920,1080,[&](const Settings&){return true;})==ProfileSwitchResult::Applied);
    assert(input_fits_mirror_profile(settings,1920,1080));
    assert(input_fits_mirror_profile(settings,1080,1920));
    assert(!input_fits_mirror_profile(settings,1920,1200));
    assert(!input_fits_mirror_profile(settings,2000,1000));
    assert(mirror_limits(settings).video_packet_bytes==2*1024*1024);
    assert(mirror_limits(settings).queue_bytes==8*1024*1024);
    Settings hevc=settings;
    assert(set_mirror_codec(hevc,MirrorCodec::ExperimentalHevc));
    assert(set_mirror_resolution(hevc,3840,2160));
    assert(input_fits_mirror_profile(hevc,3840,2160));
    assert(!input_fits_mirror_profile(hevc,4096,2160));
    assert(mirror_limits(hevc).queue_bytes==16*1024*1024);
    AdvertisedCapabilities hevc_profile=advertised_capabilities(hevc);
    assert(hevc_profile.hevc && hevc_profile.width==3840 && hevc_profile.height==2160);
    assert(!hevc_profile.hls && !hevc_profile.direct_video && hevc_profile.max_fps==30);
    assert(save_settings(settings_path,hevc));
    assert(load_settings(settings_path,loaded));
    assert(loaded.codec==MirrorCodec::ExperimentalHevc && loaded.width==3840);
    assert(set_mirror_codec(hevc,MirrorCodec::H264));
    assert(hevc.width==1280 && hevc.height==720);
    VideoWindow landscape=fit_video_window(1920,1080,1920,1080);
    assert(landscape.x==0 && landscape.y==0 && landscape.width==1920 && landscape.height==1080);
    VideoWindow portrait=fit_video_window(1080,1920,1920,1080);
    assert(portrait.x==656 && portrait.y==0 && portrait.width==607 && portrait.height==1080);
    assert(fit_video_window(0,1080,1920,1080).width==0);
    assert(save_settings(settings_path,settings));
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1920 && loaded.height==1080 && loaded.max_fps==30);
    profile=advertised_capabilities(loaded);
    assert(profile.width==1920 && profile.height==1080 && profile.max_fps==30);
    { std::ofstream old(settings_path.c_str());old<<"UXPLAY_SETTINGS_V1\nresolution=1080p\nfps=60\nhdr=auto\ndiagnostics=compact\n"; }
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1920 && loaded.height==1080 && loaded.max_fps==30); // 60 FPS fallback only
    assert(loaded.diagnostics==DiagnosticsMode::Compact);
    { std::ofstream invalid(settings_path.c_str());invalid<<"UXPLAY_SETTINGS_V1\nresolution=4k\nfps=30\n"; }
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1280 && loaded.height==720); // safe recovery profile

    UsageStats usage;
    for(unsigned i=0;i<18;++i) {
        SessionSummary s;
        s.started_utc=1700000000+i;s.duration_ms=1000+i;
        s.kind=SessionKind::Mirror;s.width=1280;s.height=720;s.codec=1;
        s.incoming_frames=30;s.feed_frames=29;s.packet_drops=i==0?1:0;s.video_frame_drops=i==0?1:0;
        s.decoder_errors=i==1?1:0;s.session_errors=i==2?1:0;
        usage.record(s);
    }
    assert(usage.successful_sessions==18 && usage.mirror_sessions==18);
    assert(usage.audio_sessions==0 && usage.direct_sessions==0);
    assert(usage.recent.size()==16 && usage.recent.front().started_utc==1700000017);
    assert(usage.recent.back().started_utc==1700000002);
    assert(usage.packet_drops==1 && usage.video_frame_drops==1 && usage.decoder_errors==1 && usage.session_errors==1);
    assert(save_usage_stats(usage_path,usage));
    UsageStats restored;
    assert(load_usage_stats(usage_path,restored));
    assert(restored.successful_sessions==18 && restored.playback_ms==usage.playback_ms);
    assert(restored.recent.size()==16 && restored.recent.front().feed_frames==29);
    restored.reset();assert(save_usage_stats(usage_path,restored));
    UsageStats empty;assert(load_usage_stats(usage_path,empty));
    assert(empty.successful_sessions==0 && empty.recent.empty());
    std::ifstream keys(keys_path.c_str());std::string key_line;
    assert(std::getline(keys,key_line) && key_line=="pairing-data-must-survive");

    remove(settings_path.c_str());remove(usage_path.c_str());remove(keys_path.c_str());rmdir(directory);
    puts("PASS: 720p/1080p profile, advertisement, portrait fit, rollback, persistence and usage state");
}
