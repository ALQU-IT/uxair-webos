#include "audio.h"
#include "h264.h"
#include "media.h"
#include "receiver.h"
#include "shell.h"
#include "identity.h"
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <fstream>
#include <string>
#include <sys/stat.h>

static long process_rss_kb() {
    std::ifstream status("/proc/self/status");
    std::string key, unit;
    long value=0;
    while(status>>key) {
        if(key=="VmRSS:") { status>>value>>unit;return value; }
        std::string rest;std::getline(status,rest);
    }
    return -1;
}

int main() {
    umask(0077);start_log("uxplay.log");
    Shell shell;if(!shell.open())return 1;
    shell.screen("Starting AirPlay receiver","");
    Receiver receiver;
    if(!receiver.start(app_directory()+"/state")) {
        shell.screen("Could not start. See uxplay.log","");
        while(shell.pump())SDL_Delay(50);
        return 1;
    }
    const char *app_id=getenv("APPID");if(!app_id)app_id=UXPLAY_APP_ID;
    Media media;AudioDecoder decoder;
    bool opened=false,have_pending=false,last_paused=false,audio_started=false;
    int width=1280,height=720;
    Packet pending;
    std::vector<uint8_t> configuration;
    uint64_t origin=0;
    int64_t audio_end=0;
    std::vector<uint8_t> silence(441*4,0);
    int64_t start=0,last_ui=0,last_stats=now_ns(),stall=0,session_started=0,app_started=last_stats;
    unsigned video_count=0,audio_count=0,decode_errors=0,late_audio=0,silence_blocks=0,invalid_video=0;
    unsigned sessions_started=0,sessions_ended=0,last_decoder_events=0;
    uint64_t last_feed_video=0,last_feed_audio=0,last_feed_video_retries=0,last_feed_audio_retries=0;
    uint64_t audio_decode_calls=0,audio_decode_ns=0,audio_decode_max_ns=0;
    auto finish=[&](){
        if(opened) {
            ++sessions_ended;
            fprintf(stderr,"[session] ended duration_s=%.1f started=%u ended=%u\n",session_started?(now_ns()-session_started)/1e9:0.0,sessions_started,sessions_ended);
        }
        media.close();decoder.reset();receiver.disconnect();opened=false;have_pending=false;
        origin=0;start=0;stall=0;session_started=0;audio_end=0;configuration.clear();last_ui=0;last_paused=false;audio_started=false;
    };
    bool running=true;
    while(running) {
        int64_t now=now_ns();
        Action action=shell.action();
        if(action==Action::Exit)break;
        if(action==Action::Back) {
            if(opened||receiver.mirror||!receiver.pin().empty())finish();else break;
        }
        if(!opened&&(action==Action::Left||action==Action::Right)) {
            receiver.adjust_audio_delay(action==Action::Right?25:-25);last_ui=0;
        }
        if(receiver.reset.exchange(false))finish();
        if(receiver.activity && now-receiver.activity>20000000000LL && (opened||receiver.mirror)) {
            fprintf(stderr,"[session] idle timeout\n");
            finish();receiver.show("Connection lost. Connect again.");
        }
        if(receiver.audio_flush.exchange(false))decoder.reset();
        if(opened) {
            media.tick();
            bool paused=receiver.paused;
            if(paused!=last_paused) {media.pause(paused);last_paused=paused;}
            if(media.failed) {finish();receiver.show("Playback error. Connect again.");}
            // Some senders negotiate audio after mirroring starts. Prime the PCM
            // track only until the first real packet arrives. Filling later gaps
            // advances audio_end past delayed RTP packets and drops real audio.
            int fills=0;
            while(opened && !audio_started && !last_paused && now-receiver.last_audio_arrival>500000000 && now-start-audio_end>100000000 && fills++<10) {
                auto r=media.feed(silence.data(),silence.size(),audio_end,false);
                if(r!=FeedResult::Accepted)break;
                audio_end+=10000000;
                ++silence_blocks;
            }
        }
        if(!opened && now-last_ui>200000000) {
            shell.screen(receiver.status(),receiver.pin(),receiver.audio_delay_ms);last_ui=now;
        }
        if(now-last_stats>5000000000LL) {
            double window_s=(now-last_stats)/1e9;
            VideoTimingSample timing=receiver.take_video_timing();
            ReceiverDiagnostics q=receiver.diagnostics();
            unsigned frame_events=media.frames.load();
            unsigned event_delta=frame_events-last_decoder_events;
            double incoming_fps=timing.frames/window_s;
            double pts_fps=timing.interval_total_ns?timing.intervals*1e9/timing.interval_total_ns:0;
            double avg_frame_ms=timing.intervals?timing.interval_total_ns/double(timing.intervals)/1e6:0;
            if(opened) fprintf(stderr,"[video] incoming_fps=%.2f pts_fps=%.2f frame_ms_avg=%.2f min=%.2f max=%.2f feed_fps=%.2f decoder_events_s=%.2f invalid=%u queue_dropped_frames=%llu\n",
                incoming_fps,pts_fps,avg_frame_ms,timing.interval_min_ns/1e6,timing.interval_max_ns/1e6,
                (media.feed_video_accepted.load()-last_feed_video)/window_s,event_delta/window_s,invalid_video,(unsigned long long)q.dropped_video_frames);
            if(opened) fprintf(stderr,"[pipeline] queue=%zu/512 packets bytes=%zu/4194304 (%.1f%%) peak=%zu packets/%zu bytes dropped_packets=%llu retries_video=%llu retries_audio=%llu feed_audio_fps=%.2f feed_avg_us=%.1f feed_max_us=%.1f\n",
                q.queue_packets,q.queue_bytes,q.queue_bytes*100.0/4194304,q.peak_queue_packets,q.peak_queue_bytes,
                (unsigned long long)q.dropped_packets,
                (unsigned long long)(media.feed_video_retries.load()-last_feed_video_retries),
                (unsigned long long)(media.feed_audio_retries.load()-last_feed_audio_retries),
                (media.feed_audio_accepted.load()-last_feed_audio)/window_s,
                media.feed_calls.load()?media.feed_time_ns.load()/double(media.feed_calls.load())/1000:0.0,media.feed_max_time_ns.load()/1000.0);
            if(opened) fprintf(stderr,"[audio] sent=%u decode_errors=%u decode_avg_us=%.1f decode_max_us=%.1f late=%u silence=%u\n",
                audio_count,decode_errors,audio_decode_calls?audio_decode_ns/double(audio_decode_calls)/1000:0.0,audio_decode_max_ns/1000.0,late_audio,silence_blocks);
            fprintf(stderr,"[health] uptime_s=%.0f session_age_s=%.1f sessions_started=%u sessions_ended=%u reset_events=%u rss_kb=%ld\n",
                (now-app_started)/1e9,session_started?(now-session_started)/1e9:0.0,sessions_started,sessions_ended,receiver.session_resets.load(),process_rss_kb());
            last_decoder_events=frame_events;
            last_feed_video=media.feed_video_accepted.load();last_feed_audio=media.feed_audio_accepted.load();
            last_feed_video_retries=media.feed_video_retries.load();last_feed_audio_retries=media.feed_audio_retries.load();
            audio_decode_calls=audio_decode_ns=audio_decode_max_ns=0;
            rotate_log();last_stats=now;
        }
        if(!have_pending) {
            // Leave future packets in the shared queue so late-arriving video can
            // precede deliberately delayed audio. Avoid head-of-line blocking.
            uint64_t latest=opened?origin+uint64_t(std::max<int64_t>(0,now-start)):UINT64_MAX;
            have_pending=receiver.pop(pending,latest);
        }
        if(!have_pending){SDL_Delay(3);continue;}
        if(!opened) {
            // Start on a random-access video frame. Pre-roll audio is intentionally
            // discarded, then both streams share this frame's remote NTP origin.
            if(!pending.video) {have_pending=false;continue;}
            auto info=inspect_h264(pending.data.data(),pending.data.size());
            if(!info.valid){++invalid_video;have_pending=false;continue;}
            if(!info.configuration.empty())configuration=info.configuration;
            if(info.width) {width=info.width;height=info.height;}
            if(!info.keyframe||configuration.empty()) {have_pending=false;continue;}
            if(!media.open(app_id,width,height,true)) {
                finish();receiver.show("Could not open the TV decoder.");continue;
            }
            opened=true;origin=pending.timestamp;start=now+200000000;stall=0;session_started=now;++sessions_started;
            video_count=audio_count=decode_errors=late_audio=silence_blocks=invalid_video=0;
            audio_decode_calls=audio_decode_ns=audio_decode_max_ns=0;
            last_decoder_events=0;last_feed_video=last_feed_audio=0;last_feed_video_retries=last_feed_audio_retries=0;
            shell.transparent();
            fprintf(stderr,"[session] %dx%d audio=%d\n",width,height,int(receiver.has_audio));
        }
        int64_t pts=pending.timestamp>=origin ? int64_t(pending.timestamp-origin) : -int64_t(origin-pending.timestamp);
        if(pts<0){have_pending=false;continue;}
        // Large clock discontinuities require a fresh AirPlay session, not an unbounded queue.
        if(pts>now-start+5000000000LL) {fprintf(stderr,"[session] timestamp discontinuity\n");finish();receiver.show("Device clock changed. Connect again.");continue;}
        if(now<start+pts) {SDL_Delay(2);continue;}
        if(pending.video && pending.codec==0) {
            auto info=inspect_h264(pending.data.data(),pending.data.size());
            if(!info.valid){++invalid_video;have_pending=false;continue;}
            if(!info.configuration.empty())configuration=info.configuration;
            if(info.width && (width!=info.width||height!=info.height)) {
                media.close();width=info.width;height=info.height;
                if(!info.keyframe || !media.open(app_id,width,height,true)) {finish();continue;}
                origin=pending.timestamp;start=now;pts=0;audio_end=0;audio_started=false;decoder.reset();
                last_decoder_events=0;last_feed_video=last_feed_audio=0;last_feed_video_retries=last_feed_audio_retries=0;
                fprintf(stderr,"[session] geometry=%dx%d\n",width,height);
            }
            pending.codec=1; // Already parsed: do not repeat on decoder back-pressure.
        }
        if(!pending.video && pending.codec!=0) {
            std::vector<uint8_t> pcm;
            int64_t decode_start=now_ns();
            if(!decoder.decode(pending.codec,pending.data.data(),pending.data.size(),pcm)) {
                uint64_t elapsed=uint64_t(std::max<int64_t>(0,now_ns()-decode_start));++audio_decode_calls;audio_decode_ns+=elapsed;audio_decode_max_ns=std::max(audio_decode_max_ns,elapsed);
                if(++decode_errors<=5)fprintf(stderr,"[audio] decode failure ct=%u\n",pending.codec);
                have_pending=false;continue;
            }
            uint64_t decode_elapsed=uint64_t(std::max<int64_t>(0,now_ns()-decode_start));++audio_decode_calls;audio_decode_ns+=decode_elapsed;audio_decode_max_ns=std::max(audio_decode_max_ns,decode_elapsed);
            pending.data.swap(pcm);pending.codec=0;
            float volume=receiver.volume;
            if(volume<1) {
                for(size_t i=0;i+1<pending.data.size();i+=2) {
                    int16_t sample=int16_t(unsigned(pending.data[i])|(unsigned(pending.data[i+1])<<8));
                    int16_t out=int16_t(sample*volume);pending.data[i]=out&255;pending.data[i+1]=(uint16_t(out)>>8)&255;
                }
            }
            if(pending.data.empty()){have_pending=false;continue;}
        }
        if(!pending.video) audio_started=true;
        if(!pending.video && pts<audio_end) {
            size_t trim=size_t((audio_end-pts)*44100/1000000000)*4;
            if(trim>=pending.data.size()){++late_audio;have_pending=false;continue;}
            pending.data.erase(pending.data.begin(),pending.data.begin()+trim);
            pending.timestamp=origin+audio_end;pts=audio_end;
        }
        auto result=media.feed(pending.data.data(),pending.data.size(),pts,pending.video);
        if(result==FeedResult::Accepted) {
            if(pending.video)++video_count;else {++audio_count;audio_end=pts+int64_t(pending.data.size()/4)*1000000000/44100;}
            have_pending=false;stall=0;
        } else if(result==FeedResult::Retry) {
            if(!stall)stall=now;
            if(now-stall>3000000000LL){finish();receiver.show("Playback stopped. Connect again.");}
            SDL_Delay(5);
        } else {finish();receiver.show("Stream failure. Connect again.");}
    }
    receiver.stop();media.close();
    fprintf(stderr,"[app] stopped cleanly\n");
    return 0;
}
