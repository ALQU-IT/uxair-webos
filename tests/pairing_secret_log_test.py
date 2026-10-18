#!/usr/bin/env python3
"""Keep the bundled RAOP PIN log below the application's configured log level."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
receiver = (ROOT / 'src/receiver.cpp').read_text()
raop = (ROOT / 'vendor/uxplay/lib/raop_handlers.h').read_text()

assert 'raop_set_log_level(raop,LOGGER_ERR)' in receiver
assert 'CLIENT MUST NOW ENTER PIN = \\"%s\\"' in raop
assert 'logger_log(raop->logger, LOGGER_INFO, "*** CLIENT MUST NOW ENTER PIN' in raop
assert 'raop_set_log_level(raop,LOGGER_INFO)' not in receiver
print('PASS: RAOP INFO messages containing pairing PINs are filtered from app logs')
