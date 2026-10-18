#pragma once
#include <string>

class PairingOverlayState {
public:
    void update_pin(const std::string& pin) { pin_ = pin.substr(0,8); }
    void completed() { pin_.clear(); }
    void cancelled() { pin_.clear(); }
    void reset() { pin_.clear(); }
    const std::string& pin() const { return pin_; }
    bool visible() const { return !pin_.empty(); }
private:
    std::string pin_;
};

enum class UiModal { None, Donation, Pairing };
inline UiModal active_ui_modal(bool pairing_pending, bool donation_open) {
    if(pairing_pending)return UiModal::Pairing;
    if(donation_open)return UiModal::Donation;
    return UiModal::None;
}
