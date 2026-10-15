#include "ui_navigation.h"
#include <cassert>
#include <cstdio>

int main() {
    UiNavigation ui;
    assert(ui.page==UiPage::Mirroring && ui.focus==0 && !ui.back());
    ui.move_category(1);assert(ui.page==UiPage::Media);
    ui.category(4);assert(ui.page==UiPage::System);
    ui.move_category(1);assert(ui.page==UiPage::Mirroring);
    ui.move_row(1,4);assert(ui.focus==1);
    ui.move_row(-1,4);assert(ui.focus==0);
    ui.begin(UiDialogKind::Resolution,1);
    ui.move_choice(-1,2);assert(ui.choice==0);
    ui.category(2);assert(ui.page==UiPage::Mirroring); // modal owns focus
    assert(ui.back() && ui.dialog==UiDialogKind::None && ui.focus==0);
    ui.begin(UiDialogKind::Resolution,1);
    assert(ui.confirm()==1 && ui.dialog==UiDialogKind::None);
    ui.category(3);ui.move_row(1,7);assert(ui.focus==1);
    ui.page=UiPage::History;assert(ui.back() && ui.page==UiPage::Statistics);
    ui.begin(UiDialogKind::ResetHistory,0);
    ui.move_choice(1,2);assert(ui.confirm()==1);
    assert(ui.back() && ui.page==UiPage::Mirroring);
    std::puts("PASS: TV sidebar, focus, choice confirmation and Back cancellation");
}
