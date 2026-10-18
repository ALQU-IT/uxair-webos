#include "ui_navigation.h"

void UiNavigation::category(int index) {
    if (dialog!=UiDialogKind::None || index<0 || index>=4) return;
    page=static_cast<UiPage>(index);
    focus=0;
}
void UiNavigation::move_category(int delta) {
    if (dialog!=UiDialogKind::None) return;
    int current=page==UiPage::History?2:static_cast<int>(page);
    category((current+delta+4)%4);
}
void UiNavigation::move_row(int delta, int count) {
    if (dialog!=UiDialogKind::None || count<=0) return;
    focus=(focus+delta+count)%count;
}
void UiNavigation::begin(UiDialogKind kind, int initial_choice) {
    if (dialog!=UiDialogKind::None || kind==UiDialogKind::None) return;
    dialog=kind;choice=initial_choice;
}
void UiNavigation::move_choice(int delta, int count) {
    if (dialog==UiDialogKind::None || count<=0) return;
    choice=(choice+delta+count)%count;
}
int UiNavigation::confirm() {
    if (dialog==UiDialogKind::None) return -1;
    int selected=choice;dialog=UiDialogKind::None;choice=0;
    return selected;
}
bool UiNavigation::back() {
    if (dialog!=UiDialogKind::None) {dialog=UiDialogKind::None;choice=0;return true;}
    if (page==UiPage::History) {page=UiPage::Diagnostics;focus=0;return true;}
    if (page!=UiPage::Mirroring) {page=UiPage::Mirroring;focus=0;return true;}
    return false;
}
