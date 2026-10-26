// ndl_backend.c - see ndl_backend.h. Found on the TV with src/ndl_probe.c:
//  * the app must be installed (the player registers on the system bus);
//  * the video is bound to the app's exported window (NDL_DirectMediaSetWindowId,
//    exported by the firmware but not by the SDK's stub library);
//  * playback only advances while audio is fed as well (silence will do).
#define _GNU_SOURCE    /* RTLD_DEFAULT */
#define NDL_DIRECTMEDIA_API_VERSION 2
#include "ndl_backend.h"
#include <NDL_directmedia.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

// NDL is initialised and bound to the video window once per process; a session
// only loads and unloads a stream. Quit + Init again between sessions loses the
// window binding ("No matching WebOSExported", "fail to create AVProxy").
static int opened, loaded;
static char bound_window[96];
static ndlb_event_fn on_event;

static void load_callback(int type, long long value, const char *message) {
    if (on_event) on_event(type, value, message);
}
static void resource_released(const char *type) {
    fprintf(stderr, "[ndl] resource released: %s\n", type ? type : "");
}

const char *ndlb_error(void) {
    const char *e = NDL_DirectMediaGetError();
    return e && *e ? e : "no error text";
}

int ndlb_open(const char *app_id, const char *window_id, int video_codec, int width, int height,
              int audio_rate, ndlb_event_fn events) {
    ndlb_close();
    on_event = events;
    if (!opened) {
        if (!NDL_DirectMedia_DL_IsInitialized() && !NDL_DirectMedia_DL_Initialize()) {
            fprintf(stderr, "[ndl] DL_Initialize failed: %s\n", ndlb_error());
            return -1;
        }
        if (NDL_DirectMediaInit(app_id, resource_released)) {
            fprintf(stderr, "[ndl] Init(%s) failed: %s\n", app_id, ndlb_error());
            return -1;
        }
        opened = 1;
        bound_window[0] = 0;
    }
    if (window_id && strcmp(window_id, bound_window)) {
        int (*set_window_id)(const char *) = (int (*)(const char *))dlsym(RTLD_DEFAULT, "NDL_DirectMediaSetWindowId");
        int r = set_window_id ? set_window_id(window_id) : -1;
        fprintf(stderr, "[ndl] SetWindowId(%s): %d%s\n", window_id, r, set_window_id ? "" : " (not exported)");
        if (!r) snprintf(bound_window, sizeof bound_window, "%s", window_id);
    }
    NDL_DIRECTMEDIA_DATA_INFO_T info;
    memset(&info, 0, sizeof info);
    if (video_codec != NDLB_VIDEO_NONE) {
        info.video.width = width;
        info.video.height = height;
        info.video.type = video_codec == NDLB_VIDEO_H265 ? NDL_VIDEO_TYPE_H265 : NDL_VIDEO_TYPE_H264;
    }
    if (audio_rate) {
        info.audio.pcm.type = NDL_AUDIO_TYPE_PCM;
        info.audio.pcm.format = NDL_DIRECTMEDIA_AUDIO_PCM_FORMAT_S16LE;
        info.audio.pcm.channelMode = NDL_DIRECTMEDIA_AUDIO_PCM_MODE_STEREO;
        info.audio.pcm.sampleRate = NDL_DIRECTMEDIA_AUDIO_PCM_SAMPLE_RATE_OF(audio_rate);
    }
    if (NDL_DirectMediaLoad(&info, load_callback)) {
        fprintf(stderr, "[ndl] Load failed: %s\n", ndlb_error());
        ndlb_close();
        return -1;
    }
    loaded = 1;
    fprintf(stderr, "[ndl] loaded video=%d %dx%d audio=%d Hz\n", video_codec, width, height, audio_rate);
    return 0;
}

int ndlb_video(const void *data, unsigned size, long long pts_us) {
    return loaded ? NDL_DirectVideoPlay((void *)data, size, pts_us) : -1;
}

int ndlb_audio(const void *data, unsigned size, long long pts_us) {
    return loaded ? NDL_DirectAudioPlay((void *)data, size, pts_us) : -1;
}

int ndlb_audio_free(void) {
    int available = -1;
    if (!loaded || NDL_DirectAudioGetAvailableBufferSize(&available)) return -1;
    return available;
}

void ndlb_close(void) {
    if (loaded) { NDL_DirectMediaUnload(); loaded = 0; fprintf(stderr, "[ndl] unloaded\n"); }
    on_event = NULL;
}

void ndlb_quit(void) {
    ndlb_close();
    if (opened) { NDL_DirectMediaQuit(); opened = 0; bound_window[0] = 0; fprintf(stderr, "[ndl] quit\n"); }
}
