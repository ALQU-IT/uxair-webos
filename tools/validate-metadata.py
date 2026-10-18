#!/usr/bin/env python3
"""Validate public-release metadata without requiring the webOS toolchain."""
import json
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
APP = json.loads((ROOT / 'packaging/appinfo.json').read_text())
EXPECTED_ID = 'com.github.depius.uxair'
EXPECTED_NAME = 'UxAir'
EXPECTED_DESCRIPTION = 'UxAir is an open-source AirPlay receiver for compatible rooted LG webOS TVs, supporting hardware-accelerated H.264 and HEVC screen mirroring.'

def fail(message):
    print(f'FAIL: {message}', file=sys.stderr)
    raise SystemExit(1)

def main():
    if APP['id'] != EXPECTED_ID:
        fail('unexpected application ID')
    if APP['title'] != EXPECTED_NAME or APP['vendor'] != EXPECTED_NAME:
        fail('title and vendor must use the public product name')
    if APP.get('appDescription') != EXPECTED_DESCRIPTION:
        fail('app description does not match the approved product description')
    if APP['main'] != 'uxair':
        fail('unexpected executable name')
    version = APP['version']
    if version != '1.0.0' or not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', version):
        fail('release candidate version must be 1.0.0 SemVer')
    changelog = (ROOT / 'CHANGELOG.md').read_text()
    if f'## [{version}]' not in changelog:
        fail('CHANGELOG does not contain the current version')
    for path in ['README.md', 'THIRD_PARTY.md', 'docs/RELEASING.md', 'docs/DEVELOPMENT.md', 'docs/RELEASE_NOTES_1.0.0_RC.md', 'docs/RELEASE_NOTES_1.0.0_RC2.md', 'docs/RELEASE_NOTES_1.0.0_RC3.md', 'docs/REPOSITORY_METADATA.md']:
        if not (ROOT / path).is_file():
            fail(f'missing required documentation: {path}')
    tracked_sources = list((ROOT / 'src').glob('*.cpp'))
    text = '\n'.join(p.read_text() for p in tracked_sources)
    if 'org.webosbrew.uxplaylg' in text or 'UxPlay LG' in text:
        fail('legacy identity remains in application source')
    icon = (ROOT / 'packaging/icon.png').read_bytes()
    if icon[:8] != b'\x89PNG\r\n\x1a\n' or struct.unpack('>II', icon[16:24]) != (80, 80):
        fail('webOS launcher icon must be a valid 80x80 PNG')
    for size in (256, 512):
        path = ROOT / f'assets/uxair-icon-{size}.png'
        if not path.is_file():
            fail(f'missing {size}px UxAir icon asset')
        data = path.read_bytes()
        if data[:8] != b'\x89PNG\r\n\x1a\n' or struct.unpack('>II', data[16:24]) != (size, size):
            fail(f'invalid {size}px UxAir icon asset')
    if not (ROOT / 'assets/uxair-icon.svg').is_file():
        fail('missing scalable UxAir icon artwork')
    funding = (ROOT / '.github/FUNDING.yml').read_text().strip()
    if funding != 'buy_me_a_coffee: depius':
        fail('GitHub funding must use the confirmed Buy Me a Coffee username')
    readme = (ROOT / 'README.md').read_text()
    if 'https://buymeacoffee.com/depius' not in readme or 'assets/buy-me-a-coffee.svg' not in readme:
        fail('README donation badge must link to the confirmed creator profile')
    if not (ROOT / 'assets/bmc-qr.bmp').is_file():
        fail('missing bundled donation QR bitmap')
    print(f'PASS: {EXPECTED_NAME} {version} metadata is consistent')

if __name__ == '__main__':
    main()
