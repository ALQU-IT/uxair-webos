#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 tools/bootstrap.py
cmake -S . -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/.cache/arm-webos-linux-gnueabi_sdk-buildroot/share/buildroot/toolchainfile.cmake"
cmake --build build --target media-probe
mkdir -p build/probe dist
ffmpeg -hide_banner -loglevel error -y -f lavfi -i testsrc2=size=1280x720:rate=30 -t 15 \
  -c:v libx264 -preset ultrafast -tune zerolatency -pix_fmt yuv420p -profile:v baseline \
  -x264-params aud=1:bframes=0:keyint=30 -f h264 build/test.h264
ffmpeg -hide_banner -loglevel error -y -f lavfi -i sine=frequency=440:sample_rate=48000 \
  -t 15 -ac 2 -c:a pcm_s16le -f s16le build/probe/test.pcm
python3 - <<'PY'
import json,re,shutil,struct
from pathlib import Path
b=Path('build/test.h264').read_bytes()
starts=[m.start() for m in re.finditer(b'\x00\x00\x00\x01\x09',b)]
assert len(starts)==450
with open('build/probe/test.avc','wb') as f:
    for a,z in zip(starts,starts[1:]+[len(b)]):
        f.write(struct.pack('<I',z-a));f.write(b[a:z])
shutil.copy('packaging/icon.png','build/probe/icon.png')
shutil.copy('build/media-probe','build/probe/media-probe')
app=json.loads(Path('packaging/appinfo.json').read_text())
Path('build/probe/appinfo.json').write_text(json.dumps(dict(
    id=app['id']+'.probe',version=app['version'],vendor='UxPlay webOS',type='native',
    main='media-probe',title='UxPlay webOS Probe',icon='icon.png',requiredMemory=64)))
PY
ares-package build/probe -o dist
app_id="$(python3 -c 'import json;print(json.load(open("packaging/appinfo.json"))["id"])')"
version="$(python3 -c 'import json;print(json.load(open("packaging/appinfo.json"))["version"])')"
echo "Run: TV_DEVICE=your-device ares-install -d \"\$TV_DEVICE\" dist/${app_id}.probe_${version}_arm.ipk"
echo "Then: TV_DEVICE=your-device ares-launch -d \"\$TV_DEVICE\" ${app_id}.probe"
