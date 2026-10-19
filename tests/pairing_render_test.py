#!/usr/bin/env python3
"""Guard the single-pass pairing modal composition and cancel wiring."""
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
shell = (ROOT / 'src/shell.cpp').read_text()
main = (ROOT / 'src/main.cpp').read_text()


def section(source, start, end):
    first = source.index(start)
    return source[first:source.index(end, first)]


page = section(shell, 'void Shell::page(', 'void Shell::overlay(')
overlay = section(shell, 'void Shell::overlay(', 'void Shell::draw_donation_modal(')
modals = section(shell, 'void Shell::draw_donation_modal(', 'void Shell::screen(')
assert page.count('SDL_RenderPresent(renderer)') == 1
assert overlay.count('SDL_RenderPresent(renderer)') == 1
assert 'draw_pairing_modal(pin,w,h)' in page and 'draw_donation_modal(w,h)' in page
assert 'draw_pairing_modal(pin,w,h)' in overlay
assert 'SDL_RenderPresent(renderer)' not in modals
assert 'shell.pairing_popup' not in main and 'shell.donation_panel' not in main
assert 'pairing_input_result(!pairing_pin.empty(),action==Action::Back' in main
profile = section(main, 'auto activate_profile=', 'const char *app_id=')
assert profile.index('receiver.stop();receiver.disconnect();') < profile.index('receiver.start(state_dir,requested)')
assert 'register_pairing_key(r.pairing_store_mutex,r.mutex' in (ROOT / 'src/receiver.cpp').read_text()
assert '[pairing] UI observed prompt=%s' in main
print('PASS: pairing renders in one composed present and Back cancels through RAOP restart')
