#include "pairing_ui.h"
#include "ui_navigation.h"
#include <cassert>
#include <cstdio>

int main() {
    PairingOverlayState pairing;
    assert(!pairing.visible());
    pairing.update_pin("123456");
    assert(pairing.visible() && pairing.pin()=="123456");
    pairing.update_pin("654321");
    assert(pairing.visible() && pairing.pin()=="654321");
    pairing.completed();
    assert(!pairing.visible());
    pairing.update_pin("111222");
    pairing.cancelled();
    assert(!pairing.visible());
    pairing.update_pin("333444");
    pairing.reset();
    assert(!pairing.visible());
    assert(active_ui_modal(false,false)==UiModal::None);
    assert(active_ui_modal(false,true)==UiModal::Donation);
    assert(active_ui_modal(true,false)==UiModal::Pairing);
    assert(active_ui_modal(true,true)==UiModal::Pairing);

    UiNavigation nav;
    nav.category(3);
    nav.move_row(1,5);
    assert(nav.page==UiPage::System && nav.focus==1);
    assert(nav.back() && nav.page==UiPage::Mirroring);
    std::puts("PASS: pairing PIN lifecycle, pairing-over-donation priority, and remote navigation state");
}
