#!/usr/bin/env python3
import hashlib
import importlib.util
import json
from pathlib import Path
import tempfile
root=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('profile_setup',root/'infrastructure/macos/first-install-profile.py')
setup=importlib.util.module_from_spec(spec);spec.loader.exec_module(setup)
bundle=json.loads((root/'src/assets/presets/BundledArrangements.json').read_text())
with tempfile.TemporaryDirectory() as directory:
    base=Path(directory);setup.seed(base,bundle)
    before={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in base.iterdir()}
    try:setup.seed(base,bundle)
    except RuntimeError:pass
    else:raise AssertionError('existing settings were overwritten')
    assert before=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in base.iterdir()}
    workspace=json.loads((base/'Workspace.json').read_text())
    assert workspace['selected_arrangement']==bundle['default']
    assert not workspace['footer']['hidden'] and workspace['header']['visible']
    assert workspace['groups']==[] and workspace['layout_mode']=='managed'
    assert workspace['native_arrangement']['kind']=='managed'
    records=json.loads((base/'Arrangements.json').read_text())['arrangements']
    assert len(records)==1 and records[0]['name']=='Example Preset' and not records[0].get('hidden',False)
    assert not any(e['id']==bundle['default'] for e in records)
    assert 'unboundWorkspaceToolbar' not in workspace['native_arrangement']['panels']
    # The compiled adapter and installer use the same existing panel recipe.
    import re
    source=(root/'src/core/resolvelayout.h').read_text()
    compiled=json.loads(re.search(r'R"layout\((.*)\)layout"',source).group(1))
    selected=next(e for e in bundle['arrangements'] if e['id']==bundle['default'])
    assert compiled==selected['workspace']
print('PASS: immutable managed Resolve Layout seed, native bands, only the selected visible user preset, recipe consistency and no overwrite')
