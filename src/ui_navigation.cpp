#include "ui_navigation.h"
#include <algorithm>

bool session_ui_redraw_due(bool opened,bool visible,int64_t last_render_ns,int64_t now_ns) {
    return opened&&visible&&(last_render_ns==0||now_ns-last_render_ns>=1000000000LL);
}
int visible_row_first(int selected,int count,int visible) {
    if(count<=visible||visible<=0)return 0;
    return std::max(0,std::min(selected-visible/2,count-visible));
}
void UiNavigation::category(int index) {
    if(dialog!=UiDialogKind::None||index<0||index>=5)return;
    page=static_cast<UiPage>(index);focus=0;
}
void UiNavigation::move_category(int delta) {
    if(dialog!=UiDialogKind::None)return;
    int current=static_cast<int>(page);
    category((current+delta+5)%5);
}
void UiNavigation::move_row(int delta,int count) {
    if(dialog!=UiDialogKind::None||count<=0)return;
    focus=(focus+delta+count)%count;
}
void UiNavigation::begin(UiDialogKind kind,int initial_choice) {
    if(dialog!=UiDialogKind::None||kind==UiDialogKind::None)return;
    dialog=kind;choice=initial_choice;
}
void UiNavigation::move_choice(int delta,int count) {
    if(dialog==UiDialogKind::None||count<=0)return;
    choice=(choice+delta+count)%count;
}
int UiNavigation::confirm() {
    if(dialog==UiDialogKind::None)return -1;
    int selected=choice;dialog=UiDialogKind::None;choice=0;return selected;
}
bool UiNavigation::back() {
    if(dialog!=UiDialogKind::None){dialog=UiDialogKind::None;choice=0;return true;}
    if(page!=UiPage::Mirroring){page=UiPage::Mirroring;focus=0;return true;}
    return false;
}
