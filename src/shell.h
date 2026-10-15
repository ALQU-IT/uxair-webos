#pragma once
#include <SDL.h>
#include "input.h"
#include "ui_navigation.h"
#include <cstdint>
#include <string>
#include <vector>
struct _TTF_Font;
struct UiRow {
    std::string label, value, note;
    bool enabled;
    UiRow(const std::string& l,const std::string& v,const std::string& n,bool e=true)
        : label(l),value(v),note(n),enabled(e) {}
};
struct UiDialog {
    std::string title, description;
    std::vector<std::string> choices;
    int selected = 0;
    int current = -1;
};
enum class UiHitKind { None, Category, Row, DialogChoice, Cancel };
struct UiHit {
    UiHitKind kind; int index;
    UiHit(UiHitKind k=UiHitKind::None,int i=-1):kind(k),index(i) {}
};
class Shell {
public:
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    bool open();
    bool pump();
    Action action();
    bool pointer_select(int& x, int& y) const;
    UiHit hit_test(int x, int y, int row_count, const UiDialog *dialog = nullptr) const;
    void screen(const std::string& status, const std::string& pin, int audio_delay_ms=100);
    void page(UiPage page, const std::string& subtitle, const std::vector<UiRow>& rows, int selected,
              const std::string& footer, const UiDialog *dialog = nullptr);
    void overlay(const std::vector<std::string>& lines);
    void transparent();
    ~Shell();
private:
    InputMapper input_{SDL_SCANCODE_UNKNOWN, SDL_SCANCODE_UNKNOWN};
    bool pointer_select_ = false;
    int pointer_x_ = 0, pointer_y_ = 0;
    _TTF_Font *font_=nullptr, *large_=nullptr, *small_=nullptr;
    void text(const std::string&,int y,_TTF_Font*,SDL_Color);
    void text_at(const std::string&,int x,int y,int max_width,_TTF_Font*,SDL_Color);
};
int64_t now_ns();
std::string app_directory();
void start_log(const char *name);
void rotate_log();
