# UxAir 1.0.0 release candidate 4

RC4 addresses a pairing-completion freeze reported on an LG OLED B7 after one cancelled pairing attempt.

## Changes

- Paired-key persistence now runs outside the receiver mutex used by the SDL thread. The persistent write remains serialized and atomic.
- Receiver restart now joins the RAOP HTTP callback thread before clearing pairing and client state, preventing an in-flight callback from updating cleared state.
- Added PIN-free transition logs for callback registration completion and the UI observing prompt display or dismissal, including callback duration.
- Added a finite-timeout, sanitizer-backed test for cancel → fresh PIN → callback registration → PIN dismissal while the UI can still read pairing state.

## Validation limits

Host and ARMv7 CI can validate the callback/state transition and package but cannot reproduce the B7's storage latency or confirm remote responsiveness. Successful pairing and post-pair AirPlay startup must still be checked on the television. The pairing key and receiver identity formats are unchanged.
