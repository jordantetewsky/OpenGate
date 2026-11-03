#!/usr/bin/env python3
"""Behavior checks for scratch-only import scope; no Resolve connection."""
import importlib.util
from pathlib import Path
import unittest

path = Path(__file__).resolve().parents[3] / 'infrastructure/macos/clipcolorroundtripprobe.py'
spec = importlib.util.spec_from_file_location('roundtrip', path)
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)


class Manager:
    def __init__(self, current=None, item_id='clip'):
        self.current = current
        self.calls = []
        self.item_id = item_id
    def GetCurrentProject(self): return self.current
    def ImportProject(self, path, name): self.calls.append(('import', path, name)); return True
    def LoadProject(self, name):
        self.calls.append(('load', name)); self.current = Project(self.item_id); return self.current
    def CloseProject(self, project): self.calls.append(('close', project)); self.current = None; return True


class Project:
    def __init__(self, item_id): self.item_id = item_id
    def GetCurrentTimeline(self): return self
    def GetUniqueId(self): return 'timeline'
    def GetItemListInTrack(self, kind, index): return [Clip(self.item_id)]


class Clip:
    def __init__(self, item_id): self.item_id = item_id
    def GetUniqueId(self): return self.item_id
    def GetStart(self): return 0
    def GetEnd(self): return 72
    def GetDuration(self): return 72
    def GetMediaPoolItem(self): return Source()
    def GetClipColor(self): return 'Orange'


class Source:
    def GetUniqueId(self): return 'source'
    def GetThirdPartyMetadata(self, key): return 'payload'


class ScopeChecks(unittest.TestCase):
    expected = dict(timeline_id='timeline', id='clip', media_id='source', start=0, end=72, duration=72)
    def test_never_switches_an_open_project(self):
        manager = Manager(current=object())
        with self.assertRaises(RuntimeError):
            probe.inspect_import(manager, Path('scratch.drp'), self.expected, 'key', 'payload')
        self.assertEqual(manager.calls, [])
    def test_only_loads_and_closes_its_unique_scratch_import(self):
        manager = Manager()
        result = probe.inspect_import(manager, Path('scratch.drp'), self.expected, 'key', 'payload')
        self.assertTrue(result['instance_association_portable'])
        self.assertTrue(manager.calls[0][2].startswith('OpenGate Color Import Probe '))
        self.assertEqual(manager.calls[0][2], manager.calls[1][1])
        self.assertEqual(manager.calls[-1][0], 'close')
        self.assertFalse(result['native_undo_verified'])
    def test_preserved_metadata_does_not_hide_changed_clip_identity(self):
        result = probe.inspect_import(Manager(item_id='new-id'), Path('scratch.drp'), self.expected, 'key', 'payload')
        self.assertTrue(result['metadata_survives'])
        self.assertFalse(result['instance_association_portable'])


if __name__ == '__main__': unittest.main()
