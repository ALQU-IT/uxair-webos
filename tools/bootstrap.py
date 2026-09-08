#!/usr/bin/env python3
"""Fetch pinned sources and the macOS ARM64 cross-toolchain; verify before extracting."""
import hashlib
import json
import pathlib
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
CACHE = ROOT / '.cache'
LOCK = json.loads((ROOT / 'dependencies.lock.json').read_text())

def run(*args, **kw):
    subprocess.run(args, check=True, **kw)

def main():
    CACHE.mkdir(exist_ok=True)
    for name, item in LOCK.items():
        if name == 'uxplay':
            target = ROOT / 'vendor' / 'uxplay'
            embedded = ROOT / 'upstream-uxplay-source.tar'
            if embedded.exists() and not (target / '.git').exists():
                if not target.exists():
                    target.parent.mkdir(exist_ok=True)
                    run('tar', '-xf', str(embedded), '-C', str(target.parent))
                continue
            if not (target / '.git').exists():
                target.parent.mkdir(exist_ok=True)
                run('git', 'clone', item['repository'], str(target))
            status = subprocess.check_output(['git', '-C', str(target), 'status', '--porcelain'], text=True)
            if status.strip():
                sys.exit('UxPlay has local modifications; preserve them before bootstrapping.')
            run('git', '-C', str(target), 'checkout', '--detach', item['commit'])
            continue
        archive = CACHE / item['archive']
        if not archive.exists():
            run('curl', '-fL', '--retry', '3', item['url'], '-o', str(archive))
        digest = hashlib.sha256(archive.read_bytes()).hexdigest()
        if digest != item['sha256']:
            sys.exit(f'Checksum mismatch: {archive}. Remove this archive and retry.')
        target = CACHE / item['directory']
        if not target.exists():
            run('tar', '-xf', str(archive), '-C', str(CACHE))
            if name == 'toolchain':
                run('bash', str(target / 'relocate-sdk.sh'))
    print('Pinned inputs verified.')

if __name__ == '__main__':
    main()
