import importlib.util,json,tempfile
from pathlib import Path
spec=importlib.util.spec_from_file_location('clean',Path(__file__).with_name('prepare-clean-arrangements.py'));clean=importlib.util.module_from_spec(spec);spec.loader.exec_module(clean)
with tempfile.TemporaryDirectory() as temp:
 base=Path(temp);(base/'Profile.json').write_text('{"active":"first-install-preview"}')
 directory=base/'Profiles/first-install-preview';directory.mkdir(parents=True)
 old=[{'id':'legacy','name':'Native Emulator','workspace':{'page':'edit','panels':{'x':{'interface_scale':1.2}}}},{'id':'custom','name':'Saved work','workspace':{'page':'edit','groups':[{'test':42}]}}]
 (directory/'Workspace.json').write_text('{"layout_mode":"native","selected_arrangement":"legacy"}');(directory/'Shortcuts.json').write_text('{"keep":"exact bytes"}')
 (directory/'Arrangements.json').write_text(json.dumps({'version':1,'arrangements':old}))
 receipt=clean.prepare(base);saved=json.loads((directory/'Arrangements.json').read_text())['arrangements'];assert all(e['hidden'] for e in saved)
 assert [{k:v for k,v in e.items() if k!='hidden'} for e in saved]==old
 assert json.loads((Path(receipt['backup'])/'Arrangements.json').read_text())['arrangements']==old
 (directory/'Workspace.json').write_text('{"groups":[{"retain":true}]}');before=(directory/'Arrangements.json').read_bytes()
 try:clean.prepare(base)
 except RuntimeError:pass
 else:raise AssertionError('custom layout was silently reset')
 assert before==(directory/'Arrangements.json').read_bytes()
print('PASS: clean arrangement hiding preserves records, backups, workspace, profile and shortcuts; modified workspace rejected')
