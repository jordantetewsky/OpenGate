#!/usr/bin/env python3
"""An open editing project must prevent all Red-probe mutation calls."""
import importlib.util
from pathlib import Path
import tempfile

root=Path(__file__).resolve().parents[3]
spec=importlib.util.spec_from_file_location('red_probe',root/'infrastructure/macos/probe-red-clip-color.py')
probe=importlib.util.module_from_spec(spec);spec.loader.exec_module(probe)
class Manager:
    def GetCurrentProject(self): return object()
    def __getattr__(self,name): raise AssertionError('Unexpected project operation: '+name)
class Resolve:
    def GetProjectManager(self): return Manager()
with tempfile.TemporaryDirectory() as temporary:
    output=Path(temporary)
    result=probe.run(Resolve(),output,execute=True)
    assert result['status']=='not-run' and result['mutated_existing_project'] is False
    assert set(p.name for p in output.iterdir())=={'receipt.json'}
    result=probe.run(Resolve(),output,execute=True,metadata=True)
    assert result['status']=='not-run' and result['mutated_existing_project'] is False
    result=probe.run(Resolve(),output,execute=False)
    assert result['project_open'] and result['status']=='not-run'
print('PASS: Red probe refuses all mutations when an existing project is open')
