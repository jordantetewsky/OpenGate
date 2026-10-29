# SPDX-License-Identifier: MIT
"""Construct a Mac payload and its exact installation/recovery inventory."""
import json
import pathlib
import plistlib
import shutil
import subprocess
from packageinputs import sha
from packagesigning import sign_code

SUPPORT = pathlib.Path('Library/Application Support/CineWaltz/OpenGate/Current')
RESOLVE = pathlib.Path('Library/Application Support/Blackmagic Design/DaVinci Resolve')


def copy(source, target):
    target.parent.mkdir(parents=True, exist_ok=True)
    subprocess.run(['/usr/bin/ditto', str(source), str(target)], check=True)


def prepare(root, artifacts, inputs, payload, scripts):
    version = inputs['version']; build = inputs['commit'][:7]
    runtime = inputs['runtime_manifest']
    packaging_commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    build_number = subprocess.check_output(['git', '-C', str(root), 'rev-list', '--count', inputs['commit']], text=True).strip()
    minimum = max((runtime['macos_deployment_target'], runtime['resolve_build_host']['minimum_macos']),
                  key=lambda s: tuple(map(int, s.split('.'))))
    metadata = {'schema': 1, 'product': 'OpenGate', 'vendor': 'CineWaltz', 'version': version,
                'build': build, 'packaging_commit': packaging_commit, 'package_build': build + '-pkg' + packaging_commit[:7], 'build_number': int(build_number), 'runtime_commit': inputs['commit'], 'platform': 'macos',
                'architecture': runtime['macos_architectures'], 'minimum_macos': minimum,
                'tested_resolve': runtime['resolve_build_host']['version'], 'settings_schema_written': 1,
                'settings_schema_read_max': 1, 'signed': 'Developer ID' if inputs.get('signing') else 'ad-hoc',
                'notarization_requested': bool(inputs.get('notary_profile')),
                'validation': 'Local candidate; runtime acceptance pending on both platforms.' if inputs.get('candidate') else 'Hidden and displayed fixtures; actual Resolve acceptance remains separate.',
                'displayed_verification': 'pending' if inputs.get('candidate') else 'passed',
                'live_macos_verified': False, 'live_windows_verified': False, 'windows_artifact_built': False,
                'foundation': 'Unbound (MIT)'}
    copy(inputs['bundle'], payload / RESOLVE / 'IOPlugins/Unbound.dvcp.bundle')
    copy(artifacts / 'OpenGate.app', payload / 'Applications/OpenGate.app')
    info = payload / 'Applications/OpenGate.app/Contents/Info.plist'
    app_info = plistlib.loads(info.read_bytes())
    app_info.update(CFBundleShortVersionString=version, CFBundleVersion=build_number, LSMinimumSystemVersion=minimum)
    info.write_bytes(plistlib.dumps(app_info))
    subprocess.run(['/usr/bin/codesign', '--force', '--sign', '-', str(info.parents[1])], check=True)
    workflow = payload / RESOLVE / 'Workflow Integration Plugins'
    copy(root / 'src/platforms/macos/OpenGate.py', workflow / 'OpenGate.py')
    commands = json.loads((artifacts / 'workflow-commands/manifest.json').read_text())
    for entry in commands:
        name = entry['file']
        if pathlib.Path(name).name != name:
            raise ValueError('Invalid workflow filename.')
        copy(artifacts / 'workflow-commands' / name, workflow / name)
    support = payload / SUPPORT; support.mkdir(parents=True)
    copy(root / 'LICENSE', support / 'LICENSE')
    copy(inputs['bundle'] / 'Contents/Resources/build-manifest.json', support / 'runtime-build-manifest.json')
    (support / 'release-manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    (support / 'README.txt').write_text(
        'OpenGate ' + version + ' prototype candidate (' + build + ')\n\n'
        'This candidate has not been live-tested inside Resolve on macOS or Windows.\n'
        'This package contains macOS binaries only; no Windows parity claim.\n'
        'Compilation and synthetic fixtures do not establish native GPU rendering or real input acceptance.\n'
        'Displayed fixture status: ' + metadata['displayed_verification'] + '.\n\n'
        'Save and quit Resolve before installation, removal or recovery.\n'
        'Fresh profiles start in Resolve Layout. Example Preset is optional.\n'
        'Existing profiles and user presets are preserved; installing a version does not reset them.\n'
        'Applications/OpenGate Tools contains double-click uninstall and support export.\n'
        'Recovery copies and install diagnostics: /Library/Application Support/CineWaltz/OpenGate/Transactions/.\n'
        'A failed/interrupted install leaves a Restore Previous OpenGate.command in its transaction directory.\n'
        'Support export includes selected event fields and install phases, not raw logs, project/media contents or arbitrary error text. Review before sharing. Nothing is uploaded automatically.\n\n'
        'Remaining live checks include Mixer custom-title dragging/control lifetime, Index acquisition readiness, native painting, physical/remapped keys and full Native/OpenGate transitions.\n'
        + ('Code and installer use Developer ID signing. Check the accompanying release manifest for notarization acceptance.\n' if inputs.get('signing') else 'The package is ad-hoc signed, unsigned as an installer, and not notarized. Downloaded copies may receive macOS security prompts.\n')
        + 'OpenGate by CineWaltz, based on Unbound (MIT); see LICENSE.\n')

    bundle = json.loads((root / 'src/assets/presets/BundledArrangements.json').read_text())
    chosen = next(e for e in bundle['arrangements'] if e['id'] == bundle['default'])
    state = dict(chosen['workspace']); state['selected_arrangement'] = chosen['id']
    presets = support / 'Presets'; presets.mkdir()
    (presets / 'Workspace.json').write_text(json.dumps(state, indent=2) + '\n')
    (presets / 'Arrangements.json').write_text(json.dumps({'version': 1, 'arrangements': [e for e in bundle['arrangements'] if not e.get('read_only')]}, indent=2) + '\n')
    replacements = {'VERSION': version, 'BUILD': build, 'MIN_MACOS': minimum,
                    'MIN_MAJOR': minimum.split('.')[0], 'RESOLVE_VERSION': metadata['tested_resolve']}
    scripts.mkdir()
    for path in (root / 'infrastructure/macos/installer').iterdir():
        if not path.is_file(): continue
        text = path.read_text()
        for key, value in replacements.items(): text = text.replace('@' + key + '@', value)
        target = scripts / path.name; target.write_text(text); target.chmod(0o755)
        if path.name not in ('preinstall', 'postinstall'):
            copy(target, support / path.name)
    tools = payload / 'Applications/OpenGate Tools'; tools.mkdir(parents=True)
    for label, command in [('Export OpenGate Support.command', 'export-support.command'),
                           ('Uninstall OpenGate.command', 'uninstall.command')]:
        wrapper = tools / label
        wrapper.write_text('#!/bin/bash\nexec /bin/bash "/' + str(SUPPORT / command) + '"\n')
        wrapper.chmod(0o755)
    # One inventory for replacement, uninstall and recovery. No user settings.
    paths = ['/Applications/OpenGate.app', '/' + str(RESOLVE / 'IOPlugins/Unbound.dvcp.bundle'), '/' + str(SUPPORT)]
    paths += ['/Applications/OpenGate Tools/Export OpenGate Support.command', '/Applications/OpenGate Tools/Uninstall OpenGate.command']
    paths += ['/' + str(RESOLVE / 'Workflow Integration Plugins' / name) for name in ['OpenGate.py'] + [e['file'] for e in commands]]
    inventory = '\n'.join(paths) + '\n'
    (scripts / 'paths.txt').write_text(inventory); (support / 'paths.txt').write_text(inventory)
    if inputs.get('signing'):
        sign_code(payload, inputs['signing'])
        metadata['signing_team'] = inputs['signing']['application']['team']
    helper = payload / RESOLVE / 'IOPlugins/Unbound.dvcp.bundle/Contents/MacOS'
    metadata['source_runtime_sha256'] = inputs.get('sha256', {})
    metadata['distributed_runtime_sha256'] = {name: sha(helper / name) for name in ('Unbound.dvcp', 'UnboundWidgetProbe.dylib')}
    (support / 'release-manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
    # Hash every regular payload file, including metadata and maintenance tools.
    hashes = ''.join(sha(path) + '\t/' + str(path.relative_to(payload)) + '\n'
                     for path in sorted(payload.rglob('*')) if path.is_file())
    (scripts / 'hashes.tsv').write_text(hashes)
    keep = {'preinstall', 'postinstall', 'transaction.sh', 'rollback.command', 'legacy-loaders.sh', 'paths.txt', 'hashes.tsv'}
    for path in scripts.iterdir():
        if path.name not in keep: path.unlink()
    return metadata
