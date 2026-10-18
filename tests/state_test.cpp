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
    assert(supported_mirror_fps(30) && supported_mirror_fps(60) && !supported_mirror_fps(24));
    assert(input_fits_mirror_profile(settings,1280,720));
    assert(input_fits_mirror_profile(settings,720,1280)); // portrait orientation
    assert(!input_fits_mirror_profile(settings,1920,1080));
    assert(!input_fits_mirror_profile(settings,1080,1920));
    assert(set_mirror_resolution(settings,3840,2160));
    assert(settings.codec==MirrorCodec::ExperimentalHevc);
    assert(settings.width==3840 && settings.height==2160);
    assert(set_mirror_resolution(settings,1280,720));
    assert(settings.codec==MirrorCodec::ExperimentalHevc); // returning to HD preserves codec
    assert(set_mirror_codec(settings,MirrorCodec::H264));
    assert(settings.width==1280 && settings.height==720);
    assert(set_mirror_resolution(settings,1920,1080));
    assert(settings.codec==MirrorCodec::H264 && settings.width==1920);
    assert(set_mirror_resolution(settings,3840,2160)); // UHD atomically enables HEVC
    assert(settings.codec==MirrorCodec::ExperimentalHevc && settings.width==3840);
    assert(set_mirror_codec(settings,MirrorCodec::H264)); // H.264 atomically returns to 1080p
    assert(settings.codec==MirrorCodec::H264 && settings.width==1920 && settings.height==1080);
    assert(set_mirror_resolution(settings,1280,720));
    assert(settings.codec==MirrorCodec::H264 && settings.width==1280);
    assert(set_mirror_fps(settings,60));
    assert(settings.max_fps==60);
    AdvertisedCapabilities h264_720_60=advertised_capabilities(settings);
    assert(h264_720_60.width==1280 && h264_720_60.height==720 && h264_720_60.max_fps==60);
    assert(!set_mirror_fps(settings,24) && settings.max_fps==60);
    assert(set_mirror_fps(settings,30));
    assert(settings.width==1280 && settings.height==720 && settings.max_fps==30);
    AdvertisedCapabilities profile=advertised_capabilities(settings);
    assert(profile.width==1280 && profile.height==720 && profile.max_fps==30);
    assert(codec_negotiation_result(settings,SenderVideoCodec::H264)==CodecNegotiationResult::Accept);
    assert(codec_negotiation_result(settings,SenderVideoCodec::Hevc)==CodecNegotiationResult::Reject);
    assert(mirror_limits(settings).video_packet_bytes==1024*1024);
    assert(mirror_limits(settings).queue_bytes==4*1024*1024);
    assert(!profile.direct_video && !profile.hls && !profile.hevc && profile.legacy_pairing);
    assert((UXPLAY_PINNED_FEATURES & (1ULL<<0))==0 && (UXPLAY_PINNED_FEATURES & (1ULL<<4))==0);
    assert((UXPLAY_PINNED_FEATURES & (1ULL<<42))==0 && (UXPLAY_PINNED_FEATURES & (1ULL<<27))!=0);
    settings.diagnostics=DiagnosticsMode::Detailed;
    assert(save_settings(settings_path,settings));
    Settings loaded;
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1280 && loaded.height==720 && loaded.max_fps==30);
    assert(loaded.diagnostics==DiagnosticsMode::Detailed);
    Settings persisted_60=settings;
    assert(set_mirror_fps(persisted_60,60));
    assert(save_settings(settings_path,persisted_60));
    assert(load_settings(settings_path,loaded) && loaded.max_fps==60);
    { std::ifstream saved(settings_path.c_str());std::string line;bool stored_rate=false,stored_hdr=false;
      while(std::getline(saved,line)) {if(line=="fps=60")stored_rate=true;if(line.find("hdr=")==0)stored_hdr=true;}
      assert(stored_rate && !stored_hdr); }
    assert(save_settings(settings_path,settings));
    int attempts=0;
    assert(switch_mirror_profile(settings,1920,1080,[&](const Settings& s){
        ++attempts;return s.width==1280;
    })==ProfileSwitchResult::Recovered);
    assert(attempts==2 && settings.width==1280); // failed activation rolls back
    assert(switch_mirror_profile(settings,1920,1080,[&](const Settings&){return false;})==ProfileSwitchResult::Unavailable);
    assert(settings.width==1280 && settings.height==720);
    assert(switch_mirror_profile(settings,3840,2160,[&](const Settings& proposed){++attempts;return proposed.codec==MirrorCodec::ExperimentalHevc;})==ProfileSwitchResult::Applied);
    assert(attempts==3 && settings.width==3840 && settings.codec==MirrorCodec::ExperimentalHevc);
    assert(switch_mirror_profile(settings,1920,1080,[&](const Settings&){return true;})==ProfileSwitchResult::Applied);
    assert(settings.width==1920 && settings.codec==MirrorCodec::ExperimentalHevc);
    assert(set_mirror_codec(settings,MirrorCodec::H264));
    assert(input_fits_mirror_profile(settings,1920,1080));
    assert(input_fits_mirror_profile(settings,1080,1920));
    Settings h264_1080_60=settings;assert(set_mirror_fps(h264_1080_60,60));
    AdvertisedCapabilities h264_profile_60=advertised_capabilities(h264_1080_60);
    assert(!h264_profile_60.hevc && h264_profile_60.width==1920 && h264_profile_60.max_fps==60);
    assert(set_mirror_resolution(h264_1080_60,1280,720) && h264_1080_60.max_fps==60);
    assert(codec_negotiation_result(settings,SenderVideoCodec::H264)==CodecNegotiationResult::Accept);
    assert(codec_negotiation_result(settings,SenderVideoCodec::Hevc)==CodecNegotiationResult::Reject);
    assert(!input_fits_mirror_profile(settings,1920,1200));
    assert(!input_fits_mirror_profile(settings,2000,1000));
    assert(mirror_limits(settings).video_packet_bytes==2*1024*1024);
    assert(mirror_limits(settings).queue_bytes==8*1024*1024);
    Settings hevc=settings;
    assert(set_mirror_codec(hevc,MirrorCodec::ExperimentalHevc));
    assert(hevc.width==1920 && hevc.height==1080); // codec selection preserves a supported size
    assert(set_mirror_resolution(hevc,1280,720));
    AdvertisedCapabilities hevc_720=advertised_capabilities(hevc);
    assert(hevc_720.hevc && hevc_720.width==1280 && hevc_720.height==720);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::Hevc)==CodecNegotiationResult::Accept);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::H264)==CodecNegotiationResult::AcceptH264Fallback);
    Settings hevc_720_60=hevc;assert(set_mirror_fps(hevc_720_60,60));
    assert(advertised_capabilities(hevc_720_60).max_fps==60);
    assert(set_mirror_resolution(hevc,1920,1080));
    AdvertisedCapabilities hevc_1080=advertised_capabilities(hevc);
    assert(hevc_1080.hevc && hevc_1080.width==1920 && hevc_1080.height==1080);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::Hevc)==CodecNegotiationResult::Accept);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::H264)==CodecNegotiationResult::AcceptH264Fallback);
    Settings hevc_1080_60=hevc;assert(set_mirror_fps(hevc_1080_60,60));
    assert(advertised_capabilities(hevc_1080_60).max_fps==60);
    assert(set_mirror_resolution(hevc,3840,2160));
    assert(input_fits_mirror_profile(hevc,3840,2160));
    assert(!input_fits_mirror_profile(hevc,4096,2160));
    assert(mirror_limits(hevc).queue_bytes==16*1024*1024);
    AdvertisedCapabilities hevc_profile=advertised_capabilities(hevc);
    assert(hevc_profile.hevc && hevc_profile.width==3840 && hevc_profile.height==2160);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::Hevc)==CodecNegotiationResult::Accept);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::Unsupported)==CodecNegotiationResult::Reject);
    assert(!hevc_profile.hls && !hevc_profile.direct_video && hevc_profile.max_fps==30);
    Settings hevc_4k_60=hevc;assert(set_mirror_fps(hevc_4k_60,60));
    AdvertisedCapabilities hevc_4k_profile_60=advertised_capabilities(hevc_4k_60);
    assert(hevc_4k_profile_60.hevc && hevc_4k_profile_60.width==3840 && hevc_4k_profile_60.max_fps==60);
    assert(mirror_limits(hevc_4k_60).queue_bytes==mirror_limits(hevc).queue_bytes); // no unmeasured buffer growth
    Settings h264_recovery_60=hevc_4k_60;
    assert(set_mirror_codec(h264_recovery_60,MirrorCodec::H264));
    assert(h264_recovery_60.width==1920 && h264_recovery_60.max_fps==60);
    assert(save_settings(settings_path,hevc));
    assert(load_settings(settings_path,loaded));
    assert(loaded.codec==MirrorCodec::ExperimentalHevc && loaded.width==3840);
    assert(set_mirror_codec(hevc,MirrorCodec::H264));
    assert(hevc.width==1920 && hevc.height==1080); // UHD -> highest supported H.264 profile
    assert(!advertised_capabilities(hevc).hevc);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::H264)==CodecNegotiationResult::Accept);
    assert(codec_negotiation_result(hevc,SenderVideoCodec::Hevc)==CodecNegotiationResult::Reject);
    VideoWindow landscape=fit_video_window(1920,1080,1920,1080);
    assert(landscape.x==0 && landscape.y==0 && landscape.width==1920 && landscape.height==1080);
    VideoWindow portrait=fit_video_window(1080,1920,1920,1080);
    assert(portrait.x==656 && portrait.y==0 && portrait.width==607 && portrait.height==1080);
    VideoWindow uhd_landscape=fit_video_window(3840,2160,1920,1080);
    assert(uhd_landscape.x==0 && uhd_landscape.y==0 && uhd_landscape.width==1920 && uhd_landscape.height==1080);
    VideoWindow uhd_portrait=fit_video_window(2160,3840,1920,1080);
    assert(uhd_portrait.x==656 && uhd_portrait.y==0 && uhd_portrait.width==607 && uhd_portrait.height==1080);
    VideoWindow portrait_sps=fit_video_window(996,2160,1920,1080);
    assert(portrait_sps.x==711 && portrait_sps.y==0 && portrait_sps.width==498 && portrait_sps.height==1080);
    assert(fit_video_window(0,1080,1920,1080).width==0);
    assert(save_settings(settings_path,settings));
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1920 && loaded.height==1080 && loaded.max_fps==30);
    profile=advertised_capabilities(loaded);
    assert(profile.width==1920 && profile.height==1080 && profile.max_fps==30);
    { std::ofstream old(settings_path.c_str());old<<"UXPLAY_SETTINGS_V1\nresolution=1080p\nfps=60\nhdr=auto\ndiagnostics=compact\n"; }
    assert(load_settings(settings_path,loaded));
    assert(loaded.width==1920 && loaded.height==1080 && loaded.max_fps==60); // legacy HDR key is ignored
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
