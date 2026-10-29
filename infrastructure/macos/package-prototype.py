#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Package clean verified runtime artifacts without installing them."""
import argparse
import html
import json
import pathlib
import plistlib
import subprocess
import tempfile
from packageinputs import load_inputs, sha
from packagesigning import identities, notarize
from packagepayload import prepare, copy, SUPPORT

root = pathlib.Path(__file__).resolve().parents[2]

def run(*args):
    subprocess.run(args, check=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--artifacts', type=pathlib.Path, default=root / 'build/macos')
    parser.add_argument('--hidden-receipt', type=pathlib.Path, required=True)
    parser.add_argument('--displayed-receipt', type=pathlib.Path)
    parser.add_argument('--candidate', action='store_true', help='Build a local candidate with displayed checks explicitly pending; never a verified release.')
    parser.add_argument('--sign', action='store_true', help='Use the unique matching Developer ID Application and Installer identities.')
    parser.add_argument('--notary-profile', help='Existing notarytool Keychain profile name; never a password or API key.')
    parser.add_argument('--version', default=(root / 'VERSION').read_text().strip())
    args = parser.parse_args()
    if args.notary_profile and not args.sign: parser.error('--notary-profile requires --sign')
    if not args.candidate and not (args.sign and args.notary_profile):
        parser.error('Distribution packages require --sign and --notary-profile; local candidates may remain unnotarized.')
    receipts = [args.hidden_receipt] + ([args.displayed_receipt] if args.displayed_receipt else [])
    inputs = load_inputs(root, args.artifacts, receipts, args.version, candidate=args.candidate)
    inputs['candidate'] = args.candidate
    inputs['signing'] = identities() if args.sign else None
    inputs['notary_profile'] = args.notary_profile
    packaging = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
    build = inputs['commit'][:7] + '-pkg' + packaging[:7]
    output = (root / 'build/macos/candidates' if args.candidate else root / 'release/mac') / (args.version + '-prototype-' + build)
    if output.exists(): raise SystemExit('Refusing to overwrite immutable release: ' + str(output))
    filename = 'OpenGate-' + args.version + ('-Candidate' if args.candidate else '-Prototype') + '-macOS-' + build + '.pkg'
    (root / 'build/macos').mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='package-', dir=root / 'build/macos') as temporary:
        work = pathlib.Path(temporary); payload = work / 'payload'; scripts = work / 'scripts'
        metadata = prepare(root, args.artifacts, inputs, payload, scripts)
        resources = work / 'resources'; resources.mkdir()
        title = 'OpenGate ' + args.version + (' Local Candidate' if args.candidate else ' Prototype')
        (resources / 'welcome.html').write_text('<html><body><h1>' + html.escape(title) + '</h1><p>Save and quit Resolve before installing. Requires macOS ' + metadata['minimum_macos'] + '+ and Resolve 21 at its standard location. Includes Apple Silicon and Intel binaries; live validation for this candidate is pending.</p><p>This candidate has not been tested inside Resolve on macOS or Windows. Displayed fixture status is recorded in the manifest; do not infer runtime acceptance from compilation.</p><p>Existing presets and arrangements are preserved. Signing and notarization status are recorded in the accompanying release manifest. Based on Unbound (MIT).</p></body></html>')
        (resources / 'conclusion.html').write_text('<html><body><h1>OpenGate installed</h1><p>Open Resolve. New users begin in Resolve Layout; Example Preset is optional. Existing users retain their last working state.</p><p>Double-click uninstall and support export in Applications/OpenGate Tools. Recovery records: /Library/Application Support/CineWaltz/OpenGate/Transactions/. Projects and media are not installation targets.</p></body></html>')
        copy(root / 'LICENSE', resources / 'LICENSE.txt')
        component = work / 'component.plist'
        # The payload has exactly one known app component. Explicit replacement
        # policy avoids inference over helpers and executable maintenance scripts.
        component.write_bytes(plistlib.dumps([{
            'RootRelativeBundlePath': 'Applications/OpenGate.app',
            'BundleHasStrictIdentifier': True, 'BundleIsRelocatable': False,
            'BundleIsVersionChecked': False, 'BundleOverwriteAction': 'upgrade'}]))
        run('/usr/bin/pkgbuild', '--root', str(payload), '--component-plist', str(component), '--scripts', str(scripts), '--ownership', 'recommended', '--identifier', 'com.cinewaltz.opengate.prototype', '--version', args.version, '--install-location', '/', str(work / 'component.pkg'))
        distribution = work / 'distribution.xml'
        distribution.write_text('''<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="1"><title>''' + html.escape(title) + '''</title>
<options customize="never" require-scripts="true" hostArchitectures="arm64,x86_64"/>
<domains enable_localSystem="true" enable_currentUserHome="false" enable_anywhere="false"/>
<welcome file="welcome.html"/><license file="LICENSE.txt"/><conclusion file="conclusion.html"/>
<volume-check><allowed-os-versions><os-version min="''' + metadata['minimum_macos'] + '''"/></allowed-os-versions></volume-check>
<choices-outline><line choice="default"/></choices-outline><choice id="default" visible="false"><pkg-ref id="com.cinewaltz.opengate.prototype"/></choice>
<pkg-ref id="com.cinewaltz.opengate.prototype" version="''' + args.version + '''" auth="Root">component.pkg</pkg-ref></installer-gui-script>''')
        signing_args = ['--sign', inputs['signing']['installer']['name'], '--timestamp'] if args.sign else []
        run('/usr/bin/productbuild', '--distribution', str(distribution), '--package-path', str(work), '--resources', str(resources), *signing_args, str(work / filename))
        metadata['notarized'] = False
        if args.sign: run('/usr/sbin/pkgutil', '--check-signature', str(work / filename))
        if args.notary_profile:
            evidence = root / 'build/macos/notarization' / build
            notary_receipt = notarize(work / filename, args.notary_profile, evidence)
            metadata['notarized'] = True
            metadata['notarization_submission'] = notary_receipt['id']
        expanded = work / 'expanded'
        run('/usr/sbin/pkgutil', '--expand-full', str(work / filename), str(expanded))
        archived_payloads = [path for path in expanded.rglob('Payload') if path.is_dir()]
        if len(archived_payloads) != 1: raise ValueError('Ambiguous archived payload.')
        for line in (scripts / 'hashes.tsv').read_text().splitlines():
            expected, path = line.split('\t', 1)
            archived = archived_payloads[0] / path.lstrip('/')
            if not archived.is_file() or sha(archived) != expected:
                raise ValueError('Archived payload differs from manifest: ' + path)
        for name, expected in metadata['distributed_runtime_sha256'].items():
            matches = list(expanded.rglob(name))
            if len(matches) != 1 or sha(matches[0]) != expected: raise ValueError('Archived runtime mismatch: ' + name)
        for name in ('uninstall.command', 'export-support.command', 'release-manifest.json'):
            if len(list(expanded.rglob(name))) != 1: raise ValueError('Missing maintenance tool: ' + name)
        output.mkdir(parents=True)
        copy(work / filename, output / filename)
        copy(payload / SUPPORT / 'README.txt', output / 'README.txt')
        (output / 'release-manifest.json').write_text(json.dumps(metadata, indent=2) + '\n')
        (output / 'SHA256SUMS.txt').write_text(''.join(sha(output / name) + '  ' + name + '\n' for name in (filename, 'README.txt', 'release-manifest.json')))
        print('PACKAGE VERIFIED:', output / filename)

if __name__ == '__main__': main()
