#!/usr/bin/env python3
"""Validate public-release metadata without requiring the webOS toolchain."""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
APP = json.loads((ROOT / 'packaging/appinfo.json').read_text())
EXPECTED_ID = 'com.github.danlflh.uxplaywebos'
EXPECTED_NAME = 'UxPlay webOS'

def fail(message):
    print(f'FAIL: {message}', file=sys.stderr)
    raise SystemExit(1)

def main():
    if APP['id'] != EXPECTED_ID:
        fail('unexpected application ID')
    if APP['title'] != EXPECTED_NAME or APP['vendor'] != EXPECTED_NAME:
        fail('title and vendor must use the public product name')
    if APP['main'] != 'uxplay-webos':
        fail('unexpected executable name')
    version = APP['version']
    if not re.fullmatch(r'(0|[1-9]\d*)\.(0|[1-9]\d*)\.(0|[1-9]\d*)', version):
        fail('version is not SemVer core format')
    changelog = (ROOT / 'CHANGELOG.md').read_text()
    if f'## [{version}]' not in changelog:
        fail('CHANGELOG does not contain the current version')
    for path in ['README.md', 'THIRD_PARTY.md', 'docs/RELEASING.md', 'docs/DEVELOPMENT.md']:
        if not (ROOT / path).is_file():
            fail(f'missing required documentation: {path}')
    tracked_sources = list((ROOT / 'src').glob('*.cpp'))
    text = '\n'.join(p.read_text() for p in tracked_sources)
    if 'org.webosbrew.uxplaylg' in text or 'UxPlay LG' in text:
        fail('legacy identity remains in application source')
    print(f'PASS: {EXPECTED_NAME} {version} metadata is consistent')

if __name__ == '__main__':
    main()
