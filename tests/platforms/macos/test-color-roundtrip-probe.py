#!/usr/bin/env python3
"""Behavior checks for scratch-only import scope; no Resolve connection."""
import importlib.util
from pathlib import Path
import unittest
import sys
from types import SimpleNamespace as Obj

path = Path(__file__).resolve().parents[3] / 'infrastructure/macos/clipcolorroundtripprobe.py'
sys.path.insert(0,str(path.parent))
from clipcolorprobescope import ScratchProjectScope
spec = importlib.util.spec_from_file_location('roundtrip', path)
probe = importlib.util.module_from_spec(spec)
spec.loader.exec_module(probe)
selection_spec = importlib.util.spec_from_file_location('selection', path.with_name('clipcolorselectionprobe.py'))
selection = importlib.util.module_from_spec(selection_spec)
selection_spec.loader.exec_module(selection)
metadata_spec = importlib.util.spec_from_file_location('metadata', path.with_name('clipcolormetadataprobe.py'))
metadata = importlib.util.module_from_spec(metadata_spec)
metadata_spec.loader.exec_module(metadata)


class Manager:
    def __init__(self, current=None, item_id='clip'):
        self.current = current
        self.calls = []
        self.item_id = item_id
    def GetCurrentProject(self): return self.current
    def ImportProject(self, path, name): self.calls.append(('import', path, name)); return True
    def LoadProject(self, name):
        self.calls.append(('load', name)); self.current = Project(self.item_id,name); return self.current
    def CloseProject(self, project): self.calls.append(('close', project)); self.current = None; return True
    def SaveProject(self): self.calls.append(('save',self.current)); return True


class Project:
    def __init__(self, item_id,name='scratch'): self.item_id,self.name = item_id,name
    def GetName(self): return self.name
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
    def test_switching_projects_blocks_scratch_save_and_cleanup(self):
        scratch=Project('clip','scratch');manager=Manager(current=scratch)
        scope=ScratchProjectScope(manager,'scratch');scope.bind(scratch)
        manager.current=Project('other','User project')
        with self.assertRaises(RuntimeError):scope.save()
        self.assertFalse(scope.cleanup())
        self.assertEqual(manager.calls,[])
    def test_project_manager_placeholder_requires_explicit_opt_in(self):
        folder=Obj(GetClipList=lambda:[],GetSubFolderList=lambda:[])
        placeholder=Obj(GetName=lambda:'Untitled Project',GetTimelineCount=lambda:0,GetMediaPool=lambda:Obj(GetRootFolder=lambda:folder))
        manager=Manager(current=placeholder)
        with self.assertRaises(RuntimeError):ScratchProjectScope(manager,'scratch').require_empty()
        ScratchProjectScope(manager,'scratch',True).require_empty()
        self.assertEqual(manager.calls,[])
    def test_default_name_with_media_is_never_an_empty_placeholder(self):
        folder=Obj(GetClipList=lambda:[object()],GetSubFolderList=lambda:[])
        project=Obj(GetName=lambda:'Untitled Project',GetTimelineCount=lambda:0,GetMediaPool=lambda:Obj(GetRootFolder=lambda:folder))
        manager=Manager(current=project)
        with self.assertRaises(RuntimeError):ScratchProjectScope(manager,'scratch',True).require_empty()
        self.assertEqual(manager.calls,[])

class SelectionChecks(unittest.TestCase):
    def inspect(self, selections):
        snapshots=iter(selections)
        timeline=Obj(GetUniqueId=lambda:'timeline',GetSelectedClips=lambda:next(snapshots))
        project=Obj(GetUniqueId=lambda:'project',GetCurrentTimeline=lambda:timeline)
        manager=Manager(current=project)
        result=selection.inspect_selection(Obj(GetProjectManager=lambda:manager))
        self.assertEqual(manager.calls,[])
        return result
    def test_reads_selected_ids_without_names_or_project_writes(self):
        result=self.inspect([[Clip('selected')],[Clip('selected')]])
        self.assertTrue(result['selection_supported'] and result['identity_consistent'])
        self.assertEqual(result['clips'][0]['id'],'selected')
        self.assertNotIn('name',result['clips'][0])
        self.assertFalse(result['project_writes'] or result['native_graphics_join_verified'] or result['native_undo_verified'])
    def test_selection_change_invalidates_the_snapshot(self):
        result=self.inspect([[Clip('before')],[Clip('after')]])
        self.assertFalse(result['identity_consistent'])
    def test_unsupported_selection_never_substitutes_a_playhead_clip(self):
        result=self.inspect([None])
        self.assertFalse(result['selection_supported'] or result['identity_consistent'])

class MetadataChecks(unittest.TestCase):
    def test_timeline_carrier_writes_only_its_namespaced_key_and_reopens_it(self):
        values={}
        host=Obj(SetThirdPartyMetadata=lambda data:values.update(data) or True,GetThirdPartyMetadata=lambda key:values.get(key))
        timeline=Obj(GetUniqueId=lambda:'timeline',GetMediaPoolItem=lambda:host)
        result=metadata.write_probe(Clip('clip'),timeline)
        self.assertTrue(result['accepted'] and result['immediate_matches'])
        self.assertEqual(list(values),[metadata.KEY])
        self.assertEqual(result['carrier'],'timeline')
        metadata.read_after_reopen(Clip('clip'),result,timeline)
        self.assertTrue(result['reopened_matches'])
    def test_missing_timeline_carrier_never_falls_back_to_source_metadata(self):
        timeline=Obj(GetUniqueId=lambda:'timeline',GetMediaPoolItem=lambda:None)
        result=metadata.write_probe(Clip('clip'),timeline)
        self.assertFalse(result['accepted'] or result['immediate_matches'])
    def test_import_checks_the_requested_timeline_carrier(self):
        reads=[]
        class TimelineProject(Project):
            def GetMediaPoolItem(self):
                return Obj(GetThirdPartyMetadata=lambda key:reads.append('timeline') or 'payload')
        class TimelineManager(Manager):
            def LoadProject(self,name):
                super().LoadProject(name);self.current=TimelineProject(self.item_id,name);return self.current
        result=probe.inspect_import(TimelineManager(),Path('scratch.drp'),ScopeChecks.expected,'key','payload',carrier='timeline')
        self.assertTrue(result['instance_association_portable'])
        self.assertEqual(reads,['timeline'])


if __name__ == '__main__': unittest.main()
