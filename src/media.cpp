#include "media.h"
#include "settings.h"
#include <StarfishMediaAPIs.h>
#include <AcbAPI.h>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <sstream>
#include <time.h>

// This firmware exports a three-argument wrapper; the NDK declaration omits taskId.
extern "C" int acb_video_info(long, const char *, long *) __asm__("AcbAPI_setMediaVideoData");
struct Media::Impl { std::mutex mutex; std::string video_info; };
static int video_info_number(const char *message, const char *field) {
    if(!message) return 0;
    std::string text(message);
    size_t pos=text.find(field);
    if(pos==std::string::npos) return 0;
    pos=text.find(':',pos);
    if(pos==std::string::npos) return 0;
    char *end=nullptr;
    long value=strtol(text.c_str()+pos+1,&end,10);
    return end!=text.c_str()+pos+1 && value>0 && value<=4096 ? int(value) : 0;
}
static int64_t monotonic_ns() { timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return int64_t(t.tv_sec)*1000000000LL+t.tv_nsec; }
Media::Media() : impl_(new Impl) {}
Media::~Media() { close(); delete impl_; }
static void acb_event(long, long, long event, long app, long play, const char *reply) {
    fprintf(stderr, "[acb] event=%ld app=%ld play=%ld %s\n", event, app, play, reply ? reply : "");
}
void Media::event(int type, int64_t value, const char *message, void *user) {
    Media *m = static_cast<Media *>(user);
    if (type == PF_EVENT_TYPE_FRAMEREADY) { ++m->frames; m->last_frame_event_ns=monotonic_ns(); return; }
    if (type == PF_EVENT_TYPE_INT_NEED_DATA || type == PF_EVENT_TYPE_INT_ENOUGH_DATA || type == PF_EVENT_TYPE_INT_BUFFERLOW || type == 0x2a) return;
    fprintf(stderr, "[smp] event=0x%x value=%lld %s\n", type, (long long)value, message ? message : "");
    if (type == PF_EVENT_TYPE_STR_RESOURCE_INFO)
        fprintf(stderr,"[media-state] resource-info event=0x%x load-completed=0\n",type);
    switch (type) {
    case PF_EVENT_TYPE_STR_STATE_UPDATE__LOADCOMPLETED:
        m->ready = true;
        fprintf(stderr,"[media-state] load-completed event=0x%x\n",type);
        break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__PLAYING:
        m->playing = true;
        fprintf(stderr,"[media-state] playing event=0x%x\n",type);
        break;
    case PF_EVENT_TYPE_STR_STATE_UPDATE__ENDOFSTREAM: m->ended = true; break;
    case PF_EVENT_TYPE_INT_ERROR:
    case PF_EVENT_TYPE_STR_ERROR: m->failed = true; break;
    case PF_EVENT_TYPE_STR_VIDEO_INFO:
        if (message) {
            m->video_info_width=video_info_number(message,"\"width\"");
            m->video_info_height=video_info_number(message,"\"height\"");
            std::lock_guard<std::mutex> lock(m->impl_->mutex);
            m->impl_->video_info = message;
            m->video_info_pending_ = true;
        }
        break;
    default: break;
    }
}
bool Media::open(const char *app_id, int width, int height, bool audio, int rate,
                 int video_source_buffer_bytes, int canvas_width, int canvas_height, MirrorCodec codec) {
    if (width<=0 || height<=0 || video_source_buffer_bytes<1048576 || video_source_buffer_bytes>2097152 ||
        (codec==MirrorCodec::H264 && int64_t(width)*height>1920*1080) ||
        (codec==MirrorCodec::ExperimentalHevc && int64_t(width)*height>3840*2160))
        return false;
    if (!canvas_width) canvas_width=width;
    if (!canvas_height) canvas_height=height;
    const VideoWindow window=fit_video_window(width,height,canvas_width,canvas_height);
    if (!window.width || !window.height) return false;
    close();
    ready = playing = ended = failed = false;
    frames = 0;
    video_info_width=video_info_height=0;
    acb_window_width=window.width;acb_window_height=window.height;
    last_frame_event_ns = 0;
    feed_video_accepted=feed_audio_accepted=0;
    feed_video_retries=feed_audio_retries=0;
    feed_video_rejected=0;
    feed_calls=0;
    feed_time_ns=feed_max_time_ns=0;
    play_attempts_=0;
    int64_t phase=monotonic_ns();
    fprintf(stderr,"[media-init] ACB create begin\n");
    acb_ = AcbAPI_create();
    fprintf(stderr,"[media-init] ACB create end ms=%.2f\n",(monotonic_ns()-phase)/1e6);
    phase=monotonic_ns();
    fprintf(stderr,"[media-init] ACB initialize begin\n");
    if (!acb_ || !AcbAPI_initialize(acb_, PLAYER_TYPE_MSE, app_id, acb_event)) {
        fprintf(stderr, "[media] ACB initialization failed\n"); close(); return false;
    }
    fprintf(stderr,"[media-init] ACB initialize end ms=%.2f\n",(monotonic_ns()-phase)/1e6);
    phase=monotonic_ns();
    fprintf(stderr,"[media-init] Starfish construct begin\n");
    api_ = new StarfishMediaAPIs(nullptr);
    fprintf(stderr,"[media-init] Starfish construct end ms=%.2f\n",(monotonic_ns()-phase)/1e6);
    phase=monotonic_ns();
    fprintf(stderr,"[media-init] ACB media ID begin\n");
    if (!AcbAPI_setMediaId(acb_, api_->getMediaID())) {
        fprintf(stderr, "[media] ACB media registration failed\n"); close(); return false;
    }
    fprintf(stderr,"[media-init] ACB media ID end ms=%.2f\n",(monotonic_ns()-phase)/1e6);
    std::ostringstream p;
    p << "{\"args\":[{\"mediaTransportType\":\"BUFFERSTREAM\",\"option\":{\"appId\":\"" << app_id
      << "\",\"queryPosition\":false,\"externalStreamingInfo\":{\"contents\":{\"codec\":{\"video\":\""
      << (codec==MirrorCodec::ExperimentalHevc?"H265":"H264") << "\"";
    if (audio) p << ",\"audio\":\"PCM\"";
    p << "},\"esInfo\":{\"pauseAtDecodeTime\":true,\"ptsToDecode\":0,\"seperatedPTS\":true,\"videoFpsValue\":30,\"videoFpsScale\":1},\"format\":\"RAW\",\"provider\":\"Chrome\"";
    if (audio) p << ",\"pcmInfo\":{\"channelMode\":\"stereo\",\"format\":\"S16LE\",\"sampleRate\":" << rate / 1000.0
                 << ",\"bitsPerSample\":16,\"layout\":\"interleaved\"}";
    p << "},\"restartStreaming\":false,\"streamQualityInfo\":true,\"streamQualityInfoNonFlushable\":true,\"totalStreamSize\":256,"
         "\"bufferingCtrInfo\":{\"preBufferByte\":0,\"bufferMinLevel\":0,\"bufferMaxLevel\":0,\"qBufferLevelAudio\":0,\"qBufferLevelVideo\":0,"
         "\"srcBufferLevelAudio\":{\"minimum\":1024,\"maximum\":131072},\"srcBufferLevelVideo\":{\"minimum\":1024,\"maximum\":" << video_source_buffer_bytes << "}}},"
         "\"adaptiveStreaming\":{\"audioOnly\":false,\"maxWidth\":" << width << ",\"maxHeight\":" << height << ",\"maxFrameRate\":30}}}]}";
    fprintf(stderr, "[media] Load %s\n", p.str().c_str());
    phase=monotonic_ns();
    loaded_ = api_->Load(p.str().c_str(), event, this);
    fprintf(stderr,"[media-init] Starfish Load end ms=%.2f success=%d\n",(monotonic_ns()-phase)/1e6,int(loaded_));
    if (!loaded_) { close(); return false; }
    phase=monotonic_ns();
    AcbAPI_setDisplayWindow(acb_, window.x, window.y, window.width, window.height, true, &task_);
    fprintf(stderr,"[media-init] ACB display window=%d,%d %dx%d source=%dx%d end ms=%.2f\n",
            window.x,window.y,window.width,window.height,width,height,(monotonic_ns()-phase)/1e6);
    return true;
}
void Media::tick() {
    if (!api_ || !loaded_ || failed) return;
    if (ready && !sink_attached_) {
        fprintf(stderr,"[media-init] ACB sink and loaded state begin\n");
        AcbAPI_setSinkType(acb_, SINK_TYPE_MAIN);
        AcbAPI_setState(acb_, APPSTATE_FOREGROUND, PLAYSTATE_LOADED, &task_);
        fprintf(stderr,"[media-init] ACB sink and loaded state end\n");
        sink_attached_ = true;
    }
    if (video_info_pending_.exchange(false)) {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        fprintf(stderr,"[media-init] ACB video info begin\n");
        int result=acb_video_info(acb_, impl_->video_info.c_str(), &task_);
        fprintf(stderr,"[media-init] ACB video info end result=%d\n",result);
    }
    if (ready && !play_requested_) {
        ++play_attempts_;
        if(play_attempts_==1 || play_attempts_%100==0)fprintf(stderr,"[media-init] Starfish Play attempt=%u\n",play_attempts_);
        play_requested_ = api_->Play();
        if(play_requested_)fprintf(stderr,"[media-init] Starfish Play accepted after %u attempts\n",play_attempts_);
    }
    if (playing && sink_attached_ && !acb_playing_) {
        fprintf(stderr,"[media-init] ACB playing state begin\n");
        AcbAPI_setState(acb_, APPSTATE_FOREGROUND, PLAYSTATE_PLAYING, &task_);
        fprintf(stderr,"[media-init] ACB playing state end\n");
        // ACB only needs the transition once; playing remains observable by the caller.
        acb_playing_=true;
    }
}
FeedResult Media::feed(const void *data, size_t size, int64_t pts_ns, bool video) {
    if (!api_ || !loaded_ || failed || !data || !size) {
        if(video)++feed_video_rejected;
        return FeedResult::Error;
    }
    struct timespec before, after;
    clock_gettime(CLOCK_MONOTONIC, &before);
    char payload[200];
    snprintf(payload, sizeof(payload), "{\"bufferAddr\":\"%p\",\"bufferSize\":%zu,\"pts\":%lld,\"esData\":%d}",
             data, size, (long long)pts_ns, video ? 1 : 2);
    std::string reply = api_->Feed(payload);
    clock_gettime(CLOCK_MONOTONIC, &after);
    int64_t elapsed_signed=int64_t(after.tv_sec-before.tv_sec)*1000000000LL+int64_t(after.tv_nsec-before.tv_nsec);
    uint64_t elapsed=elapsed_signed>0?uint64_t(elapsed_signed):0;
    ++feed_calls;
    feed_time_ns.fetch_add(elapsed);
    uint64_t previous=feed_max_time_ns.load();
    while(previous<elapsed&&!feed_max_time_ns.compare_exchange_weak(previous,elapsed)) {}
    if (reply.find("Ok") != std::string::npos) {
        (video?feed_video_accepted:feed_audio_accepted).fetch_add(1);
        return FeedResult::Accepted;
    }
    if (reply.find("BufferFull") != std::string::npos || reply.find("Pending") != std::string::npos) {
        (video?feed_video_retries:feed_audio_retries).fetch_add(1);
        return FeedResult::Retry;
    }
    fprintf(stderr, "[media] Feed rejected: %s\n", reply.c_str());
    if(video)++feed_video_rejected;
    return FeedResult::Error;
}
void Media::eos() { if (api_ && loaded_) api_->pushEOS(); }
void Media::pause(bool paused) {if(api_&&loaded_) {if(paused)api_->Pause();else api_->Play();}}
void Media::close() {
    if(acb_ && loaded_) AcbAPI_setState(acb_,APPSTATE_FOREGROUND,PLAYSTATE_UNLOADED,&task_);
    if (api_) {
        if (loaded_) { api_->Unload(); fprintf(stderr, "[media] unloaded\n"); }
        delete api_; api_ = nullptr;
    }
    if (acb_) {
        AcbAPI_finalize(acb_); AcbAPI_destroy(acb_); acb_ = 0;
        fprintf(stderr, "[media] ACB released\n");
    }
    loaded_ = sink_attached_ = play_requested_ = acb_playing_ = false;
    play_attempts_=0;
    video_info_pending_ = false;
}
