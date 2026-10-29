#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Signing selection and notarization fail closed without accessing credentials."""
import json
import pathlib
import subprocess
import tempfile
from unittest.mock import patch
from packagesigning import identities, notarize

application='1) '+('A'*40)+' "Developer ID Application: Fixture (ABCDEFGHIJ)"\n'
installer='2) '+('B'*40)+' "Developer ID Installer: Fixture (ABCDEFGHIJ)"\n'
with patch('packagesigning.subprocess.run',return_value=subprocess.CompletedProcess([],0,application+installer,'')):
    assert identities()['application']['team']=='ABCDEFGHIJ'
for listing in [application,application+application+installer,application+installer.replace('ABCDEFGHIJ','KLMNOPQRST')]:
    with patch('packagesigning.subprocess.run',return_value=subprocess.CompletedProcess([],0,listing,'')):
        try:identities()
        except ValueError:pass
        else:raise AssertionError('Ambiguous or mismatched identity accepted')
with tempfile.TemporaryDirectory(prefix='opengate-notary-test-') as temporary:
    base=pathlib.Path(temporary)
    calls=[]
    def reject(args,**kwargs):
        calls.append(args)
        return subprocess.CompletedProcess(args,0,json.dumps({'id':'12345678-1234-1234-1234-123456789abc','status':'Invalid'}),'')
    with patch('packagesigning.subprocess.run',side_effect=reject):
        try:notarize(base/'fixture.pkg','OpenGate',base/'rejected')
        except RuntimeError:pass
        else:raise AssertionError('Rejected notarization accepted')
    assert not any('stapler' in args for args in calls)
    assert (base/'rejected/submission.json').is_file()
    calls=[]
    def accept(args,**kwargs):
        calls.append(args)
        return subprocess.CompletedProcess(args,0,json.dumps({'id':'12345678-1234-1234-1234-123456789abc','status':'Accepted'}),'')
    with patch('packagesigning.subprocess.run',side_effect=accept):
        assert notarize(base/'fixture.pkg','OpenGate',base/'accepted')['status']=='Accepted'
    assert any('stapler' in args and 'staple' in args for args in calls)
    assert any('stapler' in args and 'validate' in args for args in calls)
    assert any('--assess' in args and 'install' in args for args in calls)
print('PASS: missing/ambiguous/mismatched identities rejected; rejected submissions never staple; accepted submissions require ticket and Gatekeeper validation')
