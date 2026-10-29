#!/usr/bin/env python3
"""Exercise the actual shell profile seeder without installing system files."""
import hashlib,json,pathlib,pwd,subprocess,tempfile
root=pathlib.Path(__file__).resolve().parents[2];script=root/'infrastructure/macos/installer/seed-profile.sh'
with tempfile.TemporaryDirectory(prefix='opengate-installer-') as temporary:
 base=pathlib.Path(temporary);resources=base/'resources';(resources/'Presets').mkdir(parents=True)
 bundle=json.loads((root/'src/assets/presets/BundledArrangements.json').read_text());chosen=next(e for e in bundle['arrangements'] if e['id']==bundle['default']);workspace=dict(chosen['workspace']);workspace['selected_arrangement']=chosen['id']
 (resources/'Presets/Workspace.json').write_text(json.dumps(workspace));(resources/'Presets/Arrangements.json').write_text(json.dumps({'version':1,'arrangements':[e for e in bundle['arrangements'] if not e.get('read_only')]}))
 def seed(home):subprocess.run(['/bin/bash',str(script),str(home),pwd.getpwuid(__import__('os').getuid()).pw_name,str(resources)],check=True)
 home=base/'fresh';seed(home);settings=home/'Library/Application Support/Unbound'
 assert json.loads((settings/'Workspace.json').read_text())['selected_arrangement']==bundle['default']
 assert len(json.loads((settings/'Arrangements.json').read_text())['arrangements'])==1
 (settings/'Shortcuts.json').write_text('{"existing":"binding"}');(settings/'Workspace.json').write_text('{"custom":"layout"}')
 before={p.name:p.read_bytes() for p in settings.iterdir()};seed(home);assert before=={p.name:p.read_bytes() for p in settings.iterdir()}
 for sentinel in ['Workspace.json','Arrangements.json','Profile.json']:
  home=base/sentinel;directory=home/'Library/Application Support/Unbound';directory.mkdir(parents=True);(directory/sentinel).write_text('preserve')
  seed(home);assert list(directory.iterdir())==[directory/sentinel] and (directory/sentinel).read_text()=='preserve'
 for p in (root/'infrastructure/macos/installer').iterdir():
  if p.suffix in ('.sh','.command') or p.name in ('preinstall','postinstall'): subprocess.run(['/bin/bash','-n',str(p)],check=True)
 print('PASS: shell installer fresh profile, bundled presets, existing/partial profiles and shortcuts preserved; shell syntax')
