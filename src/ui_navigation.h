#pragma once
#include <cstdint>

enum class UiPage { Mirroring, Media, History, Diagnostics, System };
enum class UiDialogKind { None, Resolution, Codec, FrameRate, Overlay, AudioDelay, HistoryPeriod, ResetHistory };

struct UiNavigation {
    UiPage page = UiPage::Mirroring;
    UiDialogKind dialog = UiDialogKind::None;
    int focus = 0;
    int choice = 0;

    void category(int index);
    void move_category(int delta);
    void move_row(int delta, int count);
    void begin(UiDialogKind kind, int initial_choice);
    void move_choice(int delta, int count);
    int confirm();
    bool back();
};

int visible_row_first(int selected, int count, int visible);
bool session_ui_redraw_due(bool opened, bool visible, int64_t last_render_ns, int64_t now_ns);
