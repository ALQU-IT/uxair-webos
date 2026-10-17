#pragma once

enum class UiPage { Mirroring, Media, Diagnostics, Statistics, System, History };
enum class UiDialogKind { None, Resolution, Codec, Hdr, Overlay, AudioDelay, ResetHistory };

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
