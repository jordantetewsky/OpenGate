#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Execute the stock JXA exporter against synthetic logs, including private fields."""
import json
import pathlib
import subprocess
import tempfile
root = pathlib.Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='opengate-support-') as temporary:
    home=pathlib.Path(temporary)
    logs=home/'Library/Logs/Unbound'; logs.mkdir(parents=True)
    event={'time':'2026-10-22T00:00:00Z','event':'panel.undock.result','build':'fixture','success':False,
           'error':'Private Project at /Users/private/media.mov','project':'Private Project',
           'widget':{'label':'Private Project'},'path':'/Users/private/media.mov'}
    (logs/'events.jsonl').write_text('invalid partial line\n'+json.dumps(event)+'\n')
    transactions=home/'Transactions';record=transactions/'12345678-1234-1234-1234-123456789ABC';record.mkdir(parents=True)
    (record/'events.log').write_text('2026-10-22T00:00:00Z failed Private Project /Users/private\n')
    output=home/'summary.json'
    subprocess.run(['/usr/bin/osascript','-l','JavaScript',str(root/'infrastructure/macos/installer/export-support.js'),str(home),str(output),str(transactions)],check=True)
    data=json.loads(output.read_text())
    assert data['installation'][0]['events'][0]['phase']=='failed'
    assert data['raw_logs_included'] is False and len(data['events'])==1
    assert data['events'][0]['success'] is False
    assert 'Private Project' not in output.read_text() and '/Users/private' not in output.read_text()
    assert not any(key in data['events'][0] for key in ('error','project','widget','path'))
    # New OpenGate logs and pre-upgrade Unbound logs must both survive export.
    current=home/'Library/Logs/OpenGate'; current.mkdir(parents=True)
    current_event=dict(event,time='2026-10-23T00:00:00Z',component='OpenGate',module='fixture',event_sequence=2)
    (current/'events.jsonl').write_text(json.dumps(current_event)+'\n')
    original=(logs/'events.jsonl').read_bytes()
    subprocess.run(['/usr/bin/osascript','-l','JavaScript',str(root/'infrastructure/macos/installer/export-support.js'),str(home),str(output),str(transactions)],check=True)
    data=json.loads(output.read_text())
    assert len(data['events'])==2 and data['events'][-1]['component']=='OpenGate'
    assert data['events'][-1]['event_sequence']==2 and data['events'][-1]['module']=='fixture'
    assert (logs/'events.jsonl').read_bytes()==original
    assert 'Private Project' not in output.read_text() and '/Users/private' not in output.read_text()
    print('PASS: support export parses partial logs and excludes private paths, project titles and nested content')
