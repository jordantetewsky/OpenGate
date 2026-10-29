#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Reject incomplete or mismatched release evidence before payload construction."""
import json
import pathlib
import tempfile
from unittest.mock import patch
from packageinputs import load_inputs
commit='1'*40
with tempfile.TemporaryDirectory(prefix='opengate-evidence-') as temporary:
    base=pathlib.Path(temporary);bundle=base/'io-helper/Unbound.dvcp.bundle'
    resources=bundle/'Contents/Resources';resources.mkdir(parents=True)
    binaries=bundle/'Contents/MacOS';binaries.mkdir()
    for name in ['Unbound.dvcp','UnboundWidgetProbe.dylib']:(binaries/name).write_bytes(b'fixture')
    (resources/'build-manifest.json').write_text(json.dumps({'commit':commit,'dirty':False}))
    def receipt(name,matrix,**overrides):
        data={'commit':commit,'matrix':matrix,'all_passed':True,'source_unchanged':True,'dirty_at_start':False,
              'tests':[{'name':'fixture','status':'PASS','exit_code':0}]};data.update(overrides)
        path=base/name;path.write_text(json.dumps(data));return path
    hidden=receipt('hidden.json','hidden');displayed=receipt('displayed.json','displayed')
    with patch('packageinputs.subprocess.check_output',return_value=''),patch('packageinputs.subprocess.run'):
        assert load_inputs(base,base,[hidden,displayed],'0.1.0')['commit']==commit
        assert load_inputs(base,base,[hidden],'0.1.0',candidate=True)['commit']==commit
        for version,paths in [('bad',[hidden,displayed]),('0.1.0',[hidden]),
                              ('0.1.0',[hidden,receipt('changed.json','displayed',source_unchanged=False)]),
                              ('0.1.0',[hidden,receipt('empty.json','displayed',tests=[])]),
                              ('0.1.0',[hidden,receipt('failed.json','displayed',tests=[{'status':'FAIL','exit_code':1}])])]:
            try:load_inputs(base,base,paths,version)
            except ValueError:pass
            else:raise AssertionError('Accepted invalid evidence')
    print('PASS: missing displayed evidence, source changes, empty/failed suites and invalid versions are rejected')
