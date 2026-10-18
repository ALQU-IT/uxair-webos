#!/usr/bin/env python3
"""Validate the isolated IPK identity, exact samples and absence of receiver state."""
import hashlib
import io
import json
import pathlib
import re
import runpy
import tarfile

ROOT = pathlib.Path(__file__).resolve().parent.parent
ar_members = runpy.run_path(str(ROOT / 'tools/validate-ipk.py'))['ar_members']

def require(test, message):
    if not test:
        raise SystemExit('HEVC package invalid: ' + message)

def main():
    app = json.loads((ROOT / 'packaging/hevc-probe-appinfo.json').read_text())
    require(app['id'] == 'com.github.depius.uxair.hevcprobe' and
            app['main'] == 'hevc-probe', 'unexpected app identity')
    stage = ROOT / 'build/hevc-app'
    require(json.loads((stage / 'appinfo.json').read_text()) == app, 'staged identity')
    ipk = ROOT / 'dist/hevc-probe' / f'{app["id"]}_{app["version"]}_arm.ipk'
    members = ar_members(ipk)
    payloads = [v for k,v in members.items() if k.startswith('data.tar')]
    require(len(payloads) == 1, 'data archive count')
    with tarfile.open(fileobj=io.BytesIO(payloads[0]), mode='r:*') as archive:
        files = {m.name.lstrip('./'): m for m in archive if m.isfile()}
        def content(suffix):
            names = [name for name in files if name == suffix or name.endswith('/'+suffix)]
            require(len(names)==1, 'missing or duplicate '+suffix)
            return archive.extractfile(files[names[0]]).read()
        require(json.loads(content('appinfo.json')) == app, 'packaged identity')
        require(content('hevc-probe') == (stage/'hevc-probe').read_bytes(), 'binary mismatch')
        require(content('lib/libstdc++.so.6'), 'missing runtime')
        info = content('build-info.txt').decode('ascii')
        require(re.fullmatch(r'version=[^\n]+\ncommit=[0-9a-f]{40}\n',info), 'source commit')
        require(info == (stage/'build-info.txt').read_text(), 'source metadata mismatch')
        for name in ('sample-1080p.hevc','sample-uhd.hevc'):
            sample=content(name)
            require(16 < len(sample) <= 12*1024*1024, 'sample size')
            require(hashlib.sha256(sample).digest() == hashlib.sha256((stage/name).read_bytes()).digest(),
                    'sample mismatch')
        forbidden={'uxplay-webos','paired.keys','server.key','device.id','uxplay.log','uxair.log','hevc-probe.log'}
        require(not any(pathlib.PurePosixPath(name).name in forbidden or
                        'state' in pathlib.PurePosixPath(name).parts for name in files),
                'receiver or private state included')
    print('PASS: separate HEVC probe IPK identity, samples and state isolation')

if __name__=='__main__':
    main()
