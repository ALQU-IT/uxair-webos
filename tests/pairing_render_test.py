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
assert 'activate_profile(settings)' in main[main.index('PairingInputResult pairing_input'):]
print('PASS: pairing renders in one composed present and Back cancels through RAOP restart')
