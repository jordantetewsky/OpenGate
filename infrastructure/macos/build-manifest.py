#!/usr/bin/env python3
"""Record only feature tips actually contained in this build's Git ancestry."""
import json
import os
import pathlib
import plistlib
import hashlib
import subprocess
import sys
root = pathlib.Path(__file__).resolve().parents[2]
def git(*args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()
lock = root/'infrastructure/macos/integration-features.json'
features = []
if lock.exists():
    declared = json.loads(lock.read_text())
    for entry in declared['features']:
        item = dict(entry)
        result = subprocess.run(['git', '-C', str(root), 'merge-base', '--is-ancestor', entry['commit'], 'HEAD'], capture_output=True)
        item['included'] = result.returncode == 0
        item['ancestry_verified'] = result.returncode in (0, 1)
        features.append(item)
manifest = {'schema': 1, 'product': 'OpenGate', 'vendor': 'CineWaltz', 'foundation': 'Unbound (MIT)', 'branch': git('branch', '--show-current'), 'commit': git('rev-parse', 'HEAD'),
            'dirty': bool(git('status', '--porcelain')), 'features': features,
            'macos_deployment_target': os.getenv('OPENGATE_MACOS_DEPLOYMENT_TARGET', 'unspecified'),
            'macos_architectures': os.getenv('OPENGATE_MACOS_ARCHS', 'host-default'),
            'meaning': 'Included means present in source ancestry, not acceptance-tested or complete.'}
resolve_app = pathlib.Path(os.getenv('OPENGATE_RESOLVE_APP', '/Applications/DaVinci Resolve/DaVinci Resolve.app'))
info_path = resolve_app/'Contents/Info.plist'
resolve_host = {'path': str(resolve_app), 'available': info_path.is_file(),
                'meaning': 'Build host only; other Resolve versions require validation.'}
if info_path.is_file():
    info_bytes = info_path.read_bytes()
    info = plistlib.loads(info_bytes)
    resolve_host.update(version=info.get('CFBundleShortVersionString'), build=info.get('CFBundleVersion'),
                        minimum_macos=info.get('LSMinimumSystemVersion'), info_sha256=hashlib.sha256(info_bytes).hexdigest())
manifest['resolve_build_host'] = resolve_host
manifest['qt_runtime_contract'] = '5.15.2'
out = pathlib.Path(sys.argv[1]); out.mkdir(parents=True, exist_ok=True)
payload = json.dumps(manifest, separators=(',', ':'))
(out/'build-manifest.json').write_text(json.dumps(manifest, indent=2)+'\n')
(out/'unboundbuildmanifest.h').write_text('#pragma once\n#define UNBOUND_FEATURE_MANIFEST '+json.dumps(payload)+'\n')
print('Recorded integration feature ancestry in', out/'build-manifest.json')
