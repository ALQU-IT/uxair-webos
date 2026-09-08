#!/usr/bin/env python3
import json
import pathlib
import shutil
import subprocess

ROOT = pathlib.Path(__file__).resolve().parent.parent
SDK = ROOT / '.cache/arm-webos-linux-gnueabi_sdk-buildroot'

def main():
    stage = ROOT / 'build/app'
    # Staging is disposable; never package test sessions or local pairing keys.
    if stage.exists():
        shutil.rmtree(stage)
    (stage / 'lib').mkdir(parents=True)
    (stage / 'licenses').mkdir()
    for name in ['appinfo.json', 'icon.png']:
        shutil.copy2(ROOT / 'packaging' / name, stage / name)
    shutil.copy2(ROOT / 'build/uxplay-webos', stage / 'uxplay-webos')
    shutil.copy2(SDK / 'arm-webos-linux-gnueabi/sysroot/usr/lib/libstdc++.so.6', stage / 'lib/libstdc++.so.6')
    for source, dest in [
        ('LICENSE', 'GPL-3.0.txt'),
        ('.cache/openssl-3.5.8/LICENSE.txt', 'OpenSSL.txt'),
        ('.cache/libplist-2.7.0/COPYING', 'libplist.txt'),
        ('.cache/ffmpeg-8.1.2/COPYING.LGPLv2.1', 'FFmpeg.txt'),
        ('vendor/uxplay/lib/llhttp/LICENSE-MIT', 'llhttp.txt'),
        ('THIRD_PARTY.md', 'THIRD_PARTY.md'),
        ('packaging/GCC-RUNTIME-EXCEPTION.txt', 'GCC-RUNTIME-EXCEPTION.txt'),
    ]:
        shutil.copy2(ROOT / source, stage / 'licenses' / dest)
    shutil.copy2(ROOT / 'dependencies.lock.json', stage / 'dependencies.lock.json')
    strip = SDK / 'bin/arm-webos-linux-gnueabi-strip'
    subprocess.run([str(strip), str(stage / 'uxplay-webos')], check=True)
    subprocess.run(['python3', str(ROOT / 'tools/verify-elf.py'), str(stage)], check=True)
    (ROOT / 'dist').mkdir(exist_ok=True)
    subprocess.run(['ares-package', str(stage), '-o', str(ROOT / 'dist')], check=True)

if __name__ == '__main__':
    main()
