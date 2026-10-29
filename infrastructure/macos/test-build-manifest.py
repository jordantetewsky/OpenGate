#!/usr/bin/env python3
"""Source lock makes integration provenance reproducible without local branches."""
import json
import os
import plistlib
import hashlib
import pathlib
import subprocess
import tempfile
root=pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as directory:
    application=pathlib.Path(directory)/'Resolve fixture.app'
    info_path=application/'Contents/Info.plist';info_path.parent.mkdir(parents=True)
    info_bytes=plistlib.dumps({'CFBundleShortVersionString':'21.fixture','CFBundleVersion':'123','LSMinimumSystemVersion':'15.0'})
    info_path.write_bytes(info_bytes)
    environment=os.environ.copy();environment['OPENGATE_RESOLVE_APP']=str(application)
    subprocess.run(['python3',str(root/'infrastructure/macos/build-manifest.py'),directory],check=True,env=environment)
    manifest=json.loads((pathlib.Path(directory)/'build-manifest.json').read_text())
    host=manifest['resolve_build_host']
    assert host['version']=='21.fixture' and host['build']=='123' and host['minimum_macos']=='15.0'
    assert host['info_sha256']==hashlib.sha256(info_bytes).hexdigest()
    assert manifest['qt_runtime_contract']=='5.15.2'
    lock=json.loads((root/'infrastructure/macos/integration-features.json').read_text())
    assert len(manifest['features']) == len(lock['features']) > 0
    for actual,expected in zip(manifest['features'],lock['features']):
        assert actual['branch']==expected['branch'] and actual['commit']==expected['commit']
        assert actual['included'] and actual['ancestry_verified']
        assert actual['acceptance']=='pending'
    print('PASS: exact integrated feature tips, scope and acceptance states match source lock')
