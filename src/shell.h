#pragma once
#include <SDL.h>
#include "input.h"
#include <cstdint>
#include <string>
#include <vector>
struct _TTF_Font;
enum class UiPage { Home, Settings, Statistics, Diagnostics, Capabilities, History };
struct UiRow {
    std::string label, value, note;
    bool enabled;
    UiRow(const std::string& l,const std::string& v,const std::string& n,bool e=true)
        : label(l),value(v),note(n),enabled(e) {}
};
class Shell {
public:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    bool open();
    bool pump();
    Action action();
    void screen(const std::string& status, const std::string& pin, int audio_delay_ms=100);
    void page(UiPage page, const std::string& subtitle, const std::vector<UiRow>& rows, int selected, const std::string& footer);
    void overlay(const std::vector<std::string>& lines);
    void transparent();
    ~Shell();
private:
    InputMapper input_{SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN};
    _TTF_Font *font_=nullptr, *large_=nullptr, *small_=nullptr;
    void text(const std::string&,int y,_TTF_Font*,SDL_Color);
    void text_at(const std::string&,int x,int y,int max_width,_TTF_Font*,SDL_Color);
};
int64_t now_ns();
std::string app_directory();
void start_log(const char *name);
void rotate_log();
