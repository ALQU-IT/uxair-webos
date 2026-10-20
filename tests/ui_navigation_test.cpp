#include "ui_navigation.h"
#include <cassert>
#include <cstdio>

int main() {
    UiNavigation ui;
    assert(ui.page==UiPage::Mirroring&&ui.focus==0&&!ui.back());
    ui.category(1);assert(ui.page==UiPage::Media);
    ui.category(2);assert(ui.page==UiPage::History);
    ui.category(3);assert(ui.page==UiPage::Diagnostics);
    ui.category(4);assert(ui.page==UiPage::System);
    ui.move_category(1);assert(ui.page==UiPage::Mirroring);
    ui.move_category(-1);assert(ui.page==UiPage::System);
    ui.category(0);ui.move_row(1,4);assert(ui.focus==1);ui.move_row(-1,4);assert(ui.focus==0);
    ui.begin(UiDialogKind::Resolution,1);ui.move_choice(-1,3);assert(ui.choice==0);
    ui.category(2);assert(ui.page==UiPage::Mirroring);
    assert(ui.back()&&ui.dialog==UiDialogKind::None&&ui.focus==0);
    ui.category(2);ui.move_row(1,102);assert(ui.focus==1);
    assert(visible_row_first(0,102,5)==0);
    assert(visible_row_first(50,102,5)==48);
    assert(visible_row_first(101,102,5)==97);
    ui.begin(UiDialogKind::HistoryPeriod,0);assert(ui.back()&&ui.dialog==UiDialogKind::None);
    ui.begin(UiDialogKind::ResetHistory,0);ui.move_choice(1,2);assert(ui.confirm()==1);
    assert(ui.back()&&ui.page==UiPage::Mirroring);
    ui.begin(UiDialogKind::FrameRate,0);ui.move_choice(1,2);assert(ui.confirm()==1);
    assert(!session_ui_redraw_due(false,true,0,0));
    assert(!session_ui_redraw_due(true,false,0,0));
    assert(session_ui_redraw_due(true,true,0,10));
    assert(!session_ui_redraw_due(true,true,100,999999999));
    assert(session_ui_redraw_due(true,true,100,1000000100));
    std::puts("PASS: five-page ordering, navigation, modal Back, long-page scrolling and redraw timing");
}
