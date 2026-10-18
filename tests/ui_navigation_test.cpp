#include "ui_navigation.h"
#include <cassert>
#include <cstdio>

int main() {
    UiNavigation ui;
    assert(ui.page==UiPage::Mirroring && ui.focus==0 && !ui.back());
    ui.move_category(1);assert(ui.page==UiPage::Media);
    ui.category(3);assert(ui.page==UiPage::System);
    ui.category(4);assert(ui.page==UiPage::System); // no fifth top-level section
    ui.move_category(1);assert(ui.page==UiPage::Mirroring);
    ui.move_row(1,4);assert(ui.focus==1);
    ui.move_row(-1,4);assert(ui.focus==0);
    ui.begin(UiDialogKind::Resolution,1);
    ui.move_choice(-1,2);assert(ui.choice==0);
    ui.category(2);assert(ui.page==UiPage::Mirroring); // modal owns focus
    assert(ui.back() && ui.dialog==UiDialogKind::None && ui.focus==0);
    ui.begin(UiDialogKind::Resolution,1);
    assert(ui.confirm()==1 && ui.dialog==UiDialogKind::None);
    ui.category(2);ui.move_row(1,7);assert(ui.focus==1);
    ui.page=UiPage::History;assert(ui.back() && ui.page==UiPage::Diagnostics);
    ui.begin(UiDialogKind::ResetHistory,0);
    ui.move_choice(1,2);assert(ui.confirm()==1);
    assert(ui.back() && ui.page==UiPage::Mirroring);
    assert(!session_ui_redraw_due(false,true,0,0));
    assert(!session_ui_redraw_due(true,false,0,0));
    assert(session_ui_redraw_due(true,true,0,10)); // input-triggered redraw
    assert(!session_ui_redraw_due(true,true,100,999999999));
    assert(session_ui_redraw_due(true,true,100,1000000100)); // one-second refresh
    std::puts("PASS: TV sidebar, focus, choice confirmation, Back cancellation and session redraw timing");
}
