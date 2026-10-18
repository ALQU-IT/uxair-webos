#pragma once
#include <cstdint>

enum class UiPage { Mirroring, Media, Diagnostics, System, History };
enum class UiDialogKind { None, Resolution, Codec, Overlay, AudioDelay, ResetHistory };

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
    // Returns false only when Back is at the top-level mirroring page.
    bool back();
};

// Session UI redraws are normally throttled to one second. A zero timestamp
// marks input or state changes that should be reflected on the next loop.
bool session_ui_redraw_due(bool opened, bool visible, int64_t last_render_ns, int64_t now_ns);
