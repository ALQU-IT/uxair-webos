#pragma once
#include <string>

enum class HdrPolicy { Auto, Sdr };
enum class DiagnosticsMode { Off, Compact, Detailed };

struct Settings {
    int width = 1280;
    int height = 720;
    int max_fps = 30;
    HdrPolicy hdr = HdrPolicy::Auto;
    DiagnosticsMode diagnostics = DiagnosticsMode::Off;
};

// Phase 1 advertises only the measured 720p/30 H.264 SDR playback path.
bool supported_mirror_resolution(int width, int height);
bool supported_mirror_fps(int fps);
bool set_mirror_resolution(Settings& settings, int width, int height);
bool set_mirror_fps(Settings& settings, int fps);
bool input_fits_mirror_profile(const Settings& settings, int width, int height);
void validate_settings(Settings& settings);
bool load_settings(const std::string& path, Settings& settings);
bool save_settings(const std::string& path, const Settings& settings);
const char *diagnostics_mode_name(DiagnosticsMode mode);
