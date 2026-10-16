#include "hevc_sample.h"
#include <StarfishMediaAPIs.h>
#include <AcbAPI.h>
#include <SDL.h>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <mutex>
#include <string>
#include <vector>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

// An isolated experiment: no receiver, discovery, sockets, pairing or persistent settings.
namespace {
const char kAppId[] = "com.github.danlflh.uxplaywebos.hevcprobe";
volatile sig_atomic_t stop_requested = 0;
void stop_signal(int) { stop_requested = 1; }
void hard_deadline(int) {
    const char message[] = "[hevc-probe] hard-deadline: synchronous firmware call stalled; process exits\n";
    write(STDERR_FILENO, message, sizeof(message)-1);
    _exit(124); // Last resort; normal paths explicitly unload and finalize.
}
int64_t now_ms() { timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return int64_t(t.tv_sec)*1000+t.tv_nsec/1000000; }
int rss_kib() {
    FILE *f=fopen("/proc/self/status","r"); if (!f) return -1;
    char line[160]; int rss=-1;
    while (fgets(line,sizeof(line),f)) if (sscanf(line,"VmRSS: %d kB",&rss)==1) break;
    fclose(f); return rss;
}
std::string self_dir() {
    char path[4096]; ssize_t n=readlink("/proc/self/exe",path,sizeof(path)-1);
    if (n<1) return "";
    path[n]=0;
    std::string s(path);
    return s.substr(0,s.rfind('/'));
}
int dimension(const char *s,const char *key) {
    if (!s) return 0;
    const char *p=strstr(s,key);
    if (!p) return 0;
    p += strlen(key); char *end=nullptr; long n=strtol(p,&end,10);
    return end!=p && n>0 && n<=8192 ? int(n) : 0;
}
struct Events {
    std::atomic<bool> loaded{false}, playing{false}, failed{false};
    std::atomic<unsigned> frame_ready{0}, errors{0}, event_logs{0};
    std::atomic<int> video_width{0}, video_height{0};
    std::mutex info_mutex;
    std::string video_info;
    std::atomic<bool> info_pending{false};
};
void event(int type,int64_t value,const char *message,void *user) {
    Events *e=static_cast<Events *>(user);
    if (type==PF_EVENT_TYPE_FRAMEREADY) { ++e->frame_ready; return; }
    if (type==PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED) e->loaded=true;
    if (type==PF_EVENT_TYPE_STR_STATE_UPDATE__PLAYING) e->playing=true;
    if (type==PF_EVENT_TYPE_INT_ERROR || type==PF_EVENT_TYPE_STR_ERROR) { e->failed=true; ++e->errors; }
    if (type==PF_EVENT_TYPE_STR_VIDEO_INFO) {
        e->video_width=dimension(message,"\"width\":");
        e->video_height=dimension(message,"\"height\":");
        if (message && strnlen(message,2048)<2048) {
            std::lock_guard<std::mutex> lock(e->info_mutex);
            e->video_info=message; e->info_pending=true;
        }
    }
    if (e->event_logs.fetch_add(1)<48)
        fprintf(stderr,"[hevc-probe] event type=0x%x value=%lld video_info=%dx%d errors=%u\n",
                type,(long long)value,e->video_width.load(),e->video_height.load(),e->errors.load());
}
void acb_event(long,long,long type,long app,long play,const char *) {
    static std::atomic<unsigned> logged{0};
    if (logged.fetch_add(1)<48)
        fprintf(stderr,"[hevc-probe] acb event=%ld app=%ld play=%ld\n",type,app,play);
}
// The NDK prototype omits the taskId that this firmware's wrapper expects.
extern "C" int acb_video_info(long,const char *,long *) __asm__("AcbAPI_setMediaVideoData");
struct Player {
    long acb=0, task=0; StarfishMediaAPIs *api=nullptr;
    bool acb_initialized=false, loaded=false, sink=false, acb_playing=false;
    Events events;
    ~Player() { close(); }
    bool open(int width,int height,int window_width,int window_height) {
        int64_t t=now_ms(); acb=AcbAPI_create();
        fprintf(stderr,"[hevc-probe] acb-create result=%d ms=%lld\n",acb!=0,(long long)(now_ms()-t));
        if (!acb) return false;
        t=now_ms(); acb_initialized=AcbAPI_initialize(acb,PLAYER_TYPE_MSE,kAppId,acb_event);
        fprintf(stderr,"[hevc-probe] acb-init result=%d ms=%lld\n",acb_initialized,(long long)(now_ms()-t));
        if (!acb_initialized) return false;
        t=now_ms(); api=new StarfishMediaAPIs(nullptr);
        fprintf(stderr,"[hevc-probe] starfish-construct result=%d ms=%lld\n",api!=nullptr,(long long)(now_ms()-t));
        if (!api || !AcbAPI_setMediaId(acb,api->getMediaID())) return false;
        char payload[1400];
        int n=snprintf(payload,sizeof(payload),
            "{\"args\":[{\"mediaTransportType\":\"BUFFERSTREAM\",\"option\":{\"appId\":\"%s\","
            "\"queryPosition\":false,\"externalStreamingInfo\":{\"contents\":{\"codec\":{\"video\":\"H265\"},"
            "\"esInfo\":{\"pauseAtDecodeTime\":true,\"ptsToDecode\":0,\"seperatedPTS\":true,"
            "\"videoFpsValue\":30,\"videoFpsScale\":1},\"format\":\"RAW\",\"provider\":\"Chrome\"},"
            "\"restartStreaming\":false,\"streamQualityInfo\":true,\"streamQualityInfoNonFlushable\":true,"
            "\"totalStreamSize\":256,\"bufferingCtrInfo\":{\"preBufferByte\":0,\"bufferMinLevel\":0,"
            "\"bufferMaxLevel\":0,\"qBufferLevelAudio\":0,\"qBufferLevelVideo\":0,"
            "\"srcBufferLevelAudio\":{\"minimum\":1024,\"maximum\":131072},"
            "\"srcBufferLevelVideo\":{\"minimum\":1024,\"maximum\":2097152}}},"
            "\"adaptiveStreaming\":{\"audioOnly\":false,\"maxWidth\":%d,\"maxHeight\":%d,\"maxFrameRate\":30}}}]}",
            kAppId,width,height);
        if (n<0 || size_t(n)>=sizeof(payload)) return false;
        fprintf(stderr,"[hevc-probe] load-begin codec=H265 transport=BUFFERSTREAM format=RAW bounds=%dx%d\n",width,height);
        t=now_ms(); loaded=api->Load(payload,event,&events);
        fprintf(stderr,"[hevc-probe] load-return accepted=%d ms=%lld rss_kib=%d\n",loaded,(long long)(now_ms()-t),rss_kib());
        if (!loaded) return false;
        t=now_ms(); int window_result=AcbAPI_setDisplayWindow(acb,0,0,window_width,window_height,true,&task);
        fprintf(stderr,"[hevc-probe] acb-window requested=%dx%d raw_result=%d ms=%lld (not output-plane proof)\n",
                window_width,window_height,window_result,(long long)(now_ms()-t));
        return true; // ACB return-code convention is not established on this firmware.
    }
    void tick() {
        if (!loaded || events.failed) return;
        if (events.loaded && !sink) {
            bool a=AcbAPI_setSinkType(acb,SINK_TYPE_MAIN);
            int b=AcbAPI_setState(acb,APPSTATE_FOREGROUND,PLAYSTATE_LOADED,&task);
            fprintf(stderr,"[hevc-probe] acb-sink=%d loaded-state-raw=%d\n",a,b);
            sink=a; if (!sink) events.failed=true;
        }
        if (events.info_pending.exchange(false)) {
            std::string info;
            { std::lock_guard<std::mutex> lock(events.info_mutex); info=events.video_info; }
            int result=acb_video_info(acb,info.c_str(),&task);
            fprintf(stderr,"[hevc-probe] acb-video-info result=%d width=%d height=%d\n",
                result,events.video_width.load(),events.video_height.load());
        }
        if (events.playing && sink && !acb_playing) {
            int b=AcbAPI_setState(acb,APPSTATE_FOREGROUND,PLAYSTATE_PLAYING,&task);
            fprintf(stderr,"[hevc-probe] acb-playing-state-raw=%d\n",b);
            acb_playing=true;
        }
    }
    void close() {
        if (acb && loaded) {
            int result=AcbAPI_setState(acb,APPSTATE_FOREGROUND,PLAYSTATE_UNLOADED,&task);
            fprintf(stderr,"[hevc-probe] cleanup acb-unloaded-raw=%d\n",result);
        }
        if (api) {
            if (loaded) { bool ok=api->Unload(); fprintf(stderr,"[hevc-probe] cleanup unload=%d\n",ok); }
            delete api; api=nullptr;
        }
        if (acb) {
            if (acb_initialized) { int result=AcbAPI_finalize(acb); fprintf(stderr,"[hevc-probe] cleanup acb-finalize-raw=%d\n",result); }
            AcbAPI_destroy(acb); fprintf(stderr,"[hevc-probe] cleanup acb-destroy-called\n"); acb=0;
        }
        loaded=false;
    }
};
}

int main(int argc,char **argv) {
    // An icon click or ordinary launch is inert. Only explicit CLI launch params run hardware.
    const bool hd=argc==2 && strcmp(argv[1],"{\"mode\":\"1080p\"}")==0;
    const bool uhd=argc==2 && strcmp(argv[1],"{\"mode\":\"uhd\"}")==0;
    if (!hd && !uhd) { fprintf(stderr,"HEVC probe idle: launch with explicit mode; see docs/MILESTONE5C.md\n"); return 0; }
    const std::string dir=self_dir(); if (dir.empty()) return 2;
    const std::string log=dir+"/hevc-probe.log";
    freopen(log.c_str(),"w",stderr); setvbuf(stderr,nullptr,_IOLBF,0);
    int null_fd=open("/dev/null",O_WRONLY);
    if (null_fd>=0) { dup2(null_fd,STDOUT_FILENO); close(null_fd); }
    signal(SIGTERM,stop_signal); signal(SIGINT,stop_signal); signal(SIGALRM,hard_deadline);
    alarm(45);
    const int width=uhd?3840:1920,height=uhd?2160:1080;
    if (uhd && access((dir+"/1080p-decoder-pass.flag").c_str(),F_OK)!=0) {
        fprintf(stderr,"[hevc-probe] uhd-blocked: complete the 1080p decoder probe first\n"); return 2;
    }
    const std::string file=dir+(uhd?"/sample-uhd.hevc":"/sample-1080p.hevc");
    std::ifstream in(file.c_str(),std::ios::binary|std::ios::ate);
    if (!in || in.tellg()<16 || in.tellg()>std::streamoff(hevc_probe::kMaxFile)) {
        fprintf(stderr,"[hevc-probe] sample-size-or-open-failed\n"); return 2;
    }
    size_t size=size_t(in.tellg()); std::vector<uint8_t> bytes(size); in.seekg(0);
    if (!in.read(reinterpret_cast<char *>(bytes.data()),size)) return 2;
    hevc_probe::Sample sample; std::string error;
    if (!hevc_probe::inspect(bytes.data(),bytes.size(),width,height,sample,error)) {
        fprintf(stderr,"[hevc-probe] sample-rejected reason=%s\n",error.c_str()); return 2;
    }
    fprintf(stderr,"[hevc-probe] sample-valid codec=HEVC profile=Main bitdepth=%d sps=%dx%d frames=%zu bytes=%zu rss_kib=%d\n",
            sample.bit_depth,sample.width,sample.height,sample.units.size(),bytes.size(),rss_kib());
    setenv("EGL_PLATFORM","wayland",0); setenv("XDG_RUNTIME_DIR","/tmp/xdg",0);
    if (SDL_Init(SDL_INIT_VIDEO)!=0) { fprintf(stderr,"[hevc-probe] sdl-init-failed\n"); return 2; }
    SDL_DisplayMode mode{}; SDL_Window *window=nullptr; SDL_Renderer *renderer=nullptr;
    int rc=1;
    do {
        if (SDL_GetCurrentDisplayMode(0,&mode)!=0) break;
        window=SDL_CreateWindow("HEVC Hardware Probe",0,0,mode.w,mode.h,SDL_WINDOW_FULLSCREEN);
        if (!window) break;
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED);
        if (!renderer) break;
        SDL_SetRenderDrawBlendMode(renderer,SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer,0,0,0,0); SDL_RenderClear(renderer); SDL_RenderPresent(renderer);
        fprintf(stderr,"[hevc-probe] sdl-canvas=%dx%d (not output-plane resolution)\n",mode.w,mode.h);
        Player player;
        if (!player.open(width,height,mode.w,mode.h)) break;
        const int64_t load_deadline=now_ms()+8000;
        int64_t play_deadline=0,next_play_ms=0;
        unsigned play_attempts=0;
        bool play_request=false;
        auto progress=[&]() {
            SDL_PumpEvents(); player.tick();
            if (player.events.loaded && !play_deadline) play_deadline=now_ms()+5000;
            if (player.events.loaded && !play_request && play_attempts<10 && now_ms()>=next_play_ms) {
                ++play_attempts;
                const int64_t t=now_ms(); play_request=player.api->Play();
                fprintf(stderr,"[hevc-probe] play-return attempt=%u accepted=%d ms=%lld\n",
                        play_attempts,play_request,(long long)(now_ms()-t));
                next_play_ms=now_ms()+100;
            }
        };
        const unsigned initial_frames=player.events.frame_ready.load();
        unsigned accepted=0,retries=0; int64_t start=now_ms();
        for (size_t i=0;i<sample.units.size() && !player.events.failed && !stop_requested;++i) {
            while (!stop_requested && now_ms()<start+int64_t(i)*1000/30) { progress(); SDL_Delay(2); }
            progress();
            if (!player.events.loaded && now_ms()>=load_deadline) break;
            const hevc_probe::AccessUnit& au=sample.units[i];
            char payload[192]; snprintf(payload,sizeof(payload),
                "{\"bufferAddr\":\"%p\",\"bufferSize\":%zu,\"pts\":%lld,\"esData\":1}",
                bytes.data()+au.offset,au.size,(long long)(int64_t(i)*1000000000/30));
            bool fed=false;
            for (unsigned attempt=0;attempt<10 && !stop_requested;++attempt) {
                int64_t t=now_ms(); std::string reply=player.api->Feed(payload);
                int64_t elapsed=now_ms()-t;
                if (reply.find("Ok")!=std::string::npos) { ++accepted; fed=true;
                    fprintf(stderr,"[hevc-probe] feed frame=%zu accepted=1 bytes=%zu ms=%lld\n",i,au.size,(long long)elapsed); break; }
                if (reply.find("BufferFull")!=std::string::npos || reply.find("Pending")!=std::string::npos) {
                    ++retries; progress(); SDL_Delay(20); continue;
                }
                fprintf(stderr,"[hevc-probe] feed frame=%zu rejected=1 ms=%lld\n",i,(long long)elapsed); break;
            }
            if (!fed) { fprintf(stderr,"[hevc-probe] feed-failed frame=%zu retries=%u\n",i,retries); break; }
        }
        if (accepted==sample.units.size()) {
            while (!player.events.playing && !player.events.failed && !stop_requested) {
                progress();
                if ((!player.events.loaded && now_ms()>=load_deadline) ||
                    (player.events.loaded && now_ms()>=play_deadline)) break;
                SDL_Delay(10);
            }
        }
        if (!player.events.loaded) fprintf(stderr,"[hevc-probe] load-event-timeout-or-error\n");
        if (!player.events.playing) fprintf(stderr,"[hevc-probe] play-event-timeout-or-error\n");
        const bool eos_accepted=player.api->pushEOS();
        fprintf(stderr,"[hevc-probe] eos-accepted=%d\n",eos_accepted);
        int64_t deadline=now_ms()+5000;
        while (!stop_requested && !player.events.failed && now_ms()<deadline) {
            progress(); SDL_Delay(20);
        }
        fprintf(stderr,"[hevc-probe] result input_accepted=%u/%zu retries=%u frame_ready_events=%u "
            "video_info=%dx%d acb_window=%dx%d errors=%u rss_kib=%d\n",accepted,sample.units.size(),retries,
            player.events.frame_ready.load()-initial_frames,player.events.video_width.load(),player.events.video_height.load(),
            mode.w,mode.h,player.events.errors.load(),rss_kib());
        fprintf(stderr,"[hevc-probe] evidence: Feed acceptance is input only; FRAMEREADY is a firmware callback; "
            "ACB window is requested geometry; native output-plane resolution remains unmeasured\n");
        const bool pass=!stop_requested && !player.events.failed && player.events.loaded &&
            player.events.playing && accepted==sample.units.size() &&
            player.events.frame_ready.load()>initial_frames &&
            player.events.video_width==width && player.events.video_height==height;
        if (pass && hd) { std::ofstream gate((dir+"/1080p-decoder-pass.flag").c_str()); gate << "decoder-path-passed\n"; }
        rc=pass?0:1;
        fprintf(stderr,"[hevc-probe] decoder-path=%s (visual confirmation still required)\n",pass?"PASS":"UNPROVEN");
    } while(false);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
    fprintf(stderr,"[hevc-probe] exit=%d\n",rc); alarm(0); return rc;
}
