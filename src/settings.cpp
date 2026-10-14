#include "settings.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

bool supported_mirror_resolution(int width, int height) { return width == 1280 && height == 720; }
bool supported_mirror_fps(int fps) { return fps == 30; }
bool set_mirror_resolution(Settings& settings, int width, int height) {
    if (!supported_mirror_resolution(width, height)) return false;
    settings.width = width; settings.height = height; return true;
}
bool set_mirror_fps(Settings& settings, int fps) {
    if (!supported_mirror_fps(fps)) return false;
    settings.max_fps = fps; return true;
}
bool input_fits_mirror_profile(const Settings& settings, int width, int height) {
    if(width<=0 || height<=0) return false;
    const int long_edge=settings.width>settings.height?settings.width:settings.height;
    const int max_area=settings.width*settings.height;
    return width<=long_edge && height<=long_edge && int64_t(width)*height<=max_area;
}
void validate_settings(Settings& settings) {
    if (!supported_mirror_resolution(settings.width, settings.height)) {
        settings.width = 1280; settings.height = 720;
    }
    if (!supported_mirror_fps(settings.max_fps)) settings.max_fps = 30;
    if (settings.hdr != HdrPolicy::Auto && settings.hdr != HdrPolicy::Sdr) settings.hdr = HdrPolicy::Auto;
    if (settings.diagnostics != DiagnosticsMode::Off && settings.diagnostics != DiagnosticsMode::Compact && settings.diagnostics != DiagnosticsMode::Detailed)
        settings.diagnostics = DiagnosticsMode::Off;
}
const char *diagnostics_mode_name(DiagnosticsMode mode) {
    switch(mode) {
    case DiagnosticsMode::Compact: return "Compact";
    case DiagnosticsMode::Detailed: return "Detailed";
    default: return "Off";
    }
}
bool load_settings(const std::string& path, Settings& settings) {
    std::ifstream in(path.c_str());
    std::string line;
    if (!std::getline(in, line) || line != "UXPLAY_SETTINGS_V1") return false;
    Settings parsed;
    unsigned lines = 0;
    while (std::getline(in, line) && ++lines <= 16) {
        if (line.size() > 80) return false;
        if (line == "resolution=720p") { parsed.width=1280; parsed.height=720; }
        else if (line == "resolution=1080p") { parsed.width=1920; parsed.height=1080; }
        else if (line == "resolution=4k") { parsed.width=3840; parsed.height=2160; }
        else if (line == "fps=30") parsed.max_fps=30;
        else if (line == "fps=60") parsed.max_fps=60;
        else if (line == "hdr=auto") parsed.hdr=HdrPolicy::Auto;
        else if (line == "hdr=sdr") parsed.hdr=HdrPolicy::Sdr;
        else if (line == "diagnostics=off") parsed.diagnostics=DiagnosticsMode::Off;
        else if (line == "diagnostics=compact") parsed.diagnostics=DiagnosticsMode::Compact;
        else if (line == "diagnostics=detailed") parsed.diagnostics=DiagnosticsMode::Detailed;
    }
    if (lines > 16) return false;
    validate_settings(parsed);
    settings=parsed;
    return true;
}
bool save_settings(const std::string& path, const Settings& settings) {
    Settings safe=settings; validate_settings(safe);
    const std::string temp=path+".tmp";
    FILE *file=fopen(temp.c_str(), "w");
    if (!file) return false;
    chmod(temp.c_str(), 0600);
    bool ok=fprintf(file,"UXPLAY_SETTINGS_V1\nresolution=720p\nfps=30\nhdr=%s\ndiagnostics=%s\n",
        safe.hdr==HdrPolicy::Sdr?"sdr":"auto",
        safe.diagnostics==DiagnosticsMode::Compact?"compact":safe.diagnostics==DiagnosticsMode::Detailed?"detailed":"off")>0;
    if (fflush(file)!=0 || fsync(fileno(file))!=0) ok=false;
    if (fclose(file)!=0) ok=false;
    if (ok && rename(temp.c_str(),path.c_str())==0) return true;
    remove(temp.c_str()); return false;
}
