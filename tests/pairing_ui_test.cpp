#include "pairing_ui.h"
#include "ui_navigation.h"
#include <cassert>
#include <cstdio>

int main() {
    PairingOverlayState pairing;
    assert(!pairing.visible());
    assert(pairing.update_pin("123456"));
    assert(pairing.visible() && pairing.pin()=="123456");
    assert(!pairing.update_pin("123456"));
    assert(pairing.update_pin("654321"));
    assert(pairing.visible() && pairing.pin()=="654321");
    assert(pairing.completed());
    assert(!pairing.visible());
    assert(!pairing.completed());
    assert(pairing.update_pin("111222999"));
    assert(pairing.pin()=="11122299");
    assert(pairing.cancelled());
    assert(!pairing.visible());
    assert(pairing.update_pin("333444"));
    assert(pairing.reset());
    assert(!pairing.visible());
    assert(!pairing.reset());
    assert(pairing_input_result(false,true,false)==PairingInputResult::PassThrough);
    assert(pairing_input_result(true,false,false)==PairingInputResult::Consumed);
    assert(pairing_input_result(true,true,false)==PairingInputResult::Cancel);
    assert(pairing_input_result(true,false,true)==PairingInputResult::PassThrough);
    assert(active_ui_modal(false,false)==UiModal::None);
    assert(active_ui_modal(false,true)==UiModal::Donation);
    assert(active_ui_modal(true,false)==UiModal::Pairing);
    assert(active_ui_modal(true,true)==UiModal::Pairing);

    UiNavigation nav;
    nav.category(4);
    nav.move_row(1,5);
    assert(nav.page==UiPage::System && nav.focus==1);
    assert(nav.back() && nav.page==UiPage::Mirroring);
    std::puts("PASS: pairing PIN change detection, cancellation/input priority, modal priority, and navigation state");
}
