# SPDX-License-Identifier: MIT
"""Developer ID signing and Keychain-based notarization; no credentials in source."""
import json
import pathlib
import re
import subprocess


def identities():
    result = subprocess.run(['/usr/bin/security', 'find-identity', '-v', '-p', 'basic'],
                            capture_output=True, text=True, check=True)
    found = {}
    for role in ('Application', 'Installer'):
        matches = re.findall(r'\d+\) ([0-9A-Fa-f]{40}) "(Developer ID ' + role + r':[^"\n]+\(([A-Z0-9]{10})\))"', result.stdout)
        if len(matches) != 1:
            raise ValueError('Select exactly one usable Developer ID ' + role + ' identity before signing.')
        fingerprint, name, team = matches[0]
        found[role.lower()] = {'fingerprint': fingerprint, 'name': name, 'team': team}
    if found['application']['team'] != found['installer']['team']:
        raise ValueError('Application and Installer signing identities must use the same developer team.')
    return found


def sign_code(payload, signing):
    helper = payload / 'Library/Application Support/Blackmagic Design/DaVinci Resolve/IOPlugins/Unbound.dvcp.bundle/Contents/MacOS'
    # Sign nested code before sealing its containing bundle. Signing the main
    # executable first seals the bundle, then changing the dylib invalidates it.
    targets = [(helper / 'UnboundWidgetProbe.dylib', 'com.cinewaltz.opengate.resolve-workspace'),
               (helper.parents[1], 'com.cinewaltz.opengate.resolve-bootstrap'),
               (payload / 'Applications/OpenGate.app', 'com.cinewaltz.opengate')]
    for path, identifier in targets:
        subprocess.run(['/usr/bin/codesign', '--force', '--sign', signing['application']['name'],
                        '--options', 'runtime', '--timestamp', '--identifier', identifier, str(path)], check=True)
        subprocess.run(['/usr/bin/codesign', '--verify', '--strict', '--verbose=2', str(path)], check=True)


def notarize(package, profile, evidence):
    evidence.mkdir(parents=True, exist_ok=False)
    result = subprocess.run(['/usr/bin/xcrun', 'notarytool', 'submit', str(package),
                             '--keychain-profile', profile, '--wait', '--output-format', 'json'],
                            capture_output=True, text=True)
    (evidence / 'submission.json').write_text(result.stdout)
    (evidence / 'tool-stderr.txt').write_text(result.stderr)
    if result.returncode:
        raise RuntimeError('Notarization submission failed; see ' + str(evidence))
    receipt = json.loads(result.stdout)
    if receipt.get('status') != 'Accepted':
        submission = receipt.get('id', '')
        if re.fullmatch(r'[0-9a-fA-F-]{36}', submission):
            subprocess.run(['/usr/bin/xcrun', 'notarytool', 'log', submission, '--keychain-profile', profile,
                            str(evidence / 'notarization-log.json')], check=False)
        raise RuntimeError('Apple did not accept the package; see ' + str(evidence))
    subprocess.run(['/usr/bin/xcrun', 'stapler', 'staple', str(package)], check=True)
    subprocess.run(['/usr/bin/xcrun', 'stapler', 'validate', str(package)], check=True)
    assessment = subprocess.run(['/usr/sbin/spctl', '--assess', '--type', 'install', '--verbose=2', str(package)],
                                capture_output=True, text=True)
    (evidence / 'gatekeeper.txt').write_text(assessment.stdout + assessment.stderr)
    if assessment.returncode:
        raise RuntimeError('Gatekeeper assessment failed; see ' + str(evidence))
    return receipt
