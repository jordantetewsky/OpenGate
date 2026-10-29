# SPDX-License-Identifier: MIT
"""Validate immutable build inputs before constructing an installer payload."""
import hashlib
import json
import pathlib
import re
import subprocess


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load_inputs(root, artifacts, receipts, version, candidate=False):
    if not re.fullmatch(r'\d+\.\d+\.\d+', version):
        raise ValueError('Version must be MAJOR.MINOR.PATCH.')
    if subprocess.check_output(['git', '-C', str(root), 'status', '--porcelain'], text=True).strip():
        raise ValueError('Commit packaging changes before constructing an immutable installer.')
    bundle = artifacts / 'io-helper/Unbound.dvcp.bundle'
    manifest = json.loads((bundle / 'Contents/Resources/build-manifest.json').read_text())
    commit = manifest['commit']
    if manifest.get('dirty') or not re.fullmatch(r'[0-9a-f]{40}', commit):
        raise ValueError('Runtime must come from a clean, identified source commit.')
    subprocess.run(['git', '-C', str(root), 'merge-base', '--is-ancestor', commit, 'HEAD'], check=True)
    if subprocess.check_output(['git', '-C', str(root), 'diff', commit, 'HEAD', '--', 'src'], text=True):
        raise ValueError('Runtime source differs from the packaging checkout.')
    evidence = []
    for path in receipts:
        receipt = json.loads(path.read_text())
        tested = receipt['commit']
        if (not re.fullmatch(r'[0-9a-f]{40}', tested) or receipt.get('dirty_at_start') or
                not receipt.get('all_passed') or not receipt.get('source_unchanged') or
                not receipt.get('tests') or any(t.get('status') != 'PASS' or t.get('exit_code') != 0 for t in receipt['tests'])):
            raise ValueError('Verification receipt is incomplete: ' + str(path))
        subprocess.run(['git', '-C', str(root), 'merge-base', '--is-ancestor', commit, tested], check=True)
        if subprocess.check_output(['git', '-C', str(root), 'diff', commit, tested, '--', 'src'], text=True):
            raise ValueError('Verified production source differs from runtime: ' + str(path))
        evidence.append(receipt)
    if not any(r.get('matrix') == 'hidden' for r in evidence):
        raise ValueError('A passing clean hidden matrix receipt is required.')
    if not candidate and not any(r.get('matrix') in ('displayed', 'functional', 'full') for r in evidence):
        raise ValueError('A passing matching displayed/native-window receipt is required.')
    files = {name: sha(bundle / 'Contents/MacOS' / name)
             for name in ('Unbound.dvcp', 'UnboundWidgetProbe.dylib')}
    for name in files:
        subprocess.run(['/usr/bin/codesign', '--verify', '--strict', str(bundle / 'Contents/MacOS' / name)], check=True)
    return {'commit': commit, 'bundle': bundle, 'sha256': files, 'verification': evidence,
            'runtime_manifest': manifest, 'version': version}
