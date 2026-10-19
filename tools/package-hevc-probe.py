#!/usr/bin/env python3
"""Stage only the separate HEVC test app, two local samples, and its runtime."""
import hashlib
import json
import pathlib
import shutil
import subprocess

ROOT = pathlib.Path(__file__).resolve().parent.parent
SDK = ROOT / '.cache/arm-webos-linux-gnueabi_sdk-buildroot'
APP = json.loads((ROOT / 'packaging/hevc-probe-appinfo.json').read_text())

def main():
    stage = ROOT / 'build/hevc-app'
    if stage.exists():
        shutil.rmtree(stage)
    (stage / 'lib').mkdir(parents=True)
    (stage / 'licenses').mkdir()
    (stage / 'appinfo.json').write_text(json.dumps(APP, indent=2) + '\n')
    commit = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    (stage / 'build-info.txt').write_text(f'version={APP["version"]}\ncommit={commit}\n')
    shutil.copy2(ROOT / 'packaging/icon.png', stage / 'icon.png')
    shutil.copy2(ROOT / 'build/hevc-cross/hevc-probe', stage / 'hevc-probe')
    shutil.copy2(SDK / 'arm-webos-linux-gnueabi/sysroot/usr/lib/libstdc++.so.6', stage / 'lib/libstdc++.so.6')
    shutil.copy2(ROOT / 'LICENSE', stage / 'licenses/GPL-3.0.txt')
    shutil.copy2(ROOT / 'packaging/GCC-RUNTIME-EXCEPTION.txt', stage / 'licenses/GCC-RUNTIME-EXCEPTION.txt')
    dist = ROOT / 'dist/hevc-probe'
    dist.mkdir(parents=True, exist_ok=True)
    lines = []
    for name in ('sample-1080p.hevc', 'sample-uhd.hevc'):
        src = ROOT / 'build/hevc-samples' / name
        if not 16 < src.stat().st_size <= 12 * 1024 * 1024:
            raise SystemExit(f'Sample bound exceeded: {name}')
        shutil.copy2(src, stage / name)
        shutil.copy2(src, dist / name)
        lines.append(f'{hashlib.sha256(src.read_bytes()).hexdigest()}  {name}')
    subprocess.run([str(SDK / 'bin/arm-webos-linux-gnueabi-strip'), str(stage / 'hevc-probe')], check=True)
    subprocess.run(['python3', str(ROOT / 'tools/verify-elf.py'), str(stage), 'hevc-probe'], check=True)
    subprocess.run(['ares-package', str(stage), '-o', str(dist)], check=True)
    ipk = dist / f'{APP["id"]}_{APP["version"]}_arm.ipk'
    if not ipk.is_file() or not ipk.stat().st_size:
        raise SystemExit('Expected separate HEVC probe IPK was not produced')
    lines.append(f'{hashlib.sha256(ipk.read_bytes()).hexdigest()}  {ipk.name}')
    (dist / 'SHA256SUMS').write_text('\n'.join(lines) + '\n')
    (dist / 'BUILD_COMMIT.txt').write_text(commit + '\n')
    print(f'Packaged {ipk}')

if __name__ == '__main__':
    main()
