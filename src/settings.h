#pragma once
#include <cstddef>
#include <functional>
#include <string>

enum class HdrPolicy { Auto, Sdr };
enum class DiagnosticsMode { Off, Compact, Detailed };
enum class MirrorCodec { H264, ExperimentalHevc };

struct Settings {
    int width = 1280;
    int height = 720;
    int max_fps = 30;
    HdrPolicy hdr = HdrPolicy::Auto;
    DiagnosticsMode diagnostics = DiagnosticsMode::Off;
    MirrorCodec codec = MirrorCodec::H264;
};

struct MirrorLimits {
    size_t video_packet_bytes, queue_bytes;
    int video_source_buffer_bytes;
};
struct VideoWindow { int x, y, width, height; };
enum class ProfileSwitchResult { Unchanged, Applied, Recovered, Unavailable };

// Only H.264 at 30 FPS is enabled. 1080p is experimental until B7 tested.
bool supported_mirror_resolution(int width, int height);
bool supported_mirror_fps(int fps);
bool set_mirror_resolution(Settings& settings, int width, int height);
bool set_mirror_fps(Settings& settings, int fps);
bool set_mirror_codec(Settings& settings, MirrorCodec codec);
bool input_fits_mirror_profile(const Settings& settings, int width, int height);
MirrorLimits mirror_limits(const Settings& settings);
VideoWindow fit_video_window(int source_width, int source_height, int canvas_width, int canvas_height);
ProfileSwitchResult switch_mirror_profile(Settings& current, int width, int height,
    const std::function<bool(const Settings&)>& activate);
void validate_settings(Settings& settings);
bool load_settings(const std::string& path, Settings& settings);
bool save_settings(const std::string& path, const Settings& settings);
const char *diagnostics_mode_name(DiagnosticsMode mode);
