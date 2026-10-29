"""Read-only production planning behavior; native execution is covered by Qt fixtures."""
import json,sys
from pathlib import Path
from types import SimpleNamespace as Obj
import unittest
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'src/core/operations'))
import audio_channel_plan as planning

class Clip:
    def __init__(self, identity, channel=1, count=4, kind='audio'):
        self.identity, self.kind = identity, kind
        self.mapping = {'embedded_audio_channels': count, 'linked_audio': {},
                        'track_mapping': {'1': {'channel_idx': [channel], 'mute': False, 'type': 'mono'}}}
        self.source_mapping = {"embedded_audio_channels": count, "linked_audio": {},
                               "track_mapping": {str(i): {"channel_idx": [i], "mute": False, "type": "mono"} for i in range(1, count + 1)}}
        self.writes = []
        self.path = '/available.wav'
        self.reject = False
        self.stale = False
    def GetUniqueId(self): return self.identity
    def GetTrackTypeAndIndex(self): return [self.kind, 1]
    def GetMediaPoolItem(self): return Obj(GetClipProperty=lambda name: self.path, GetAudioMapping=lambda: json.dumps(self.source_mapping))
    def GetSourceAudioChannelMapping(self): return json.dumps(self.mapping)
    def SetSourceAudioChannelMapping(self, value):
        self.writes.append(json.loads(value))
        if self.reject: return False
        if not self.stale: self.mapping.update(json.loads(value))
        return True
    def channel(self): return self.mapping['track_mapping']['1']['channel_idx'][0]


class AudioChannelTests(unittest.TestCase):
    def setUp(self):
        self.selected=[]
        self.timeline=Obj(GetSelectedClips=lambda:self.selected,GetUniqueId=lambda:'timeline')
        self.project=Obj(GetCurrentTimeline=lambda:self.timeline,GetUniqueId=lambda:'project')
        self.resolve=Obj(GetCurrentPage=lambda:'edit',GetProjectManager=lambda:Obj(GetCurrentProject=lambda:self.project))
    def plan(self,direction=1,scope='all-source',available=lambda path:True):
        result=planning.plan_selected_audio(self.resolve,direction,available,scope)
        self.assertTrue(all(not c.writes for c in self.selected))
        return result
    def test_both_directions_and_wrap_from_fresh_current_mapping(self):
        item=Clip('a',1,4);self.selected=[item]
        for before,direction,after in [(1,1,2),(4,1,1),(1,-1,4),(4,-1,3),(2,1,3)]:
            item.mapping['track_mapping']['1']['channel_idx']=[before]
            entry=self.plan(direction)['items'][0]
            self.assertEqual((entry['before'],entry['after']),(before,after))
            self.assertEqual(item.channel(),before)
    def test_mode_switch_keeps_exclusions_and_uses_current_channel(self):
        item=Clip('a',1,5);self.selected=[item]
        item.source_mapping['track_mapping']={str(i):{'channel_idx':[i],'type':'mono','mute':False} for i in (3,4,5)}
        source=json.dumps(item.source_mapping)
        for before,direction,scope,after in [(1,1,'skip-excluded',3),(2,-1,'skip-excluded',5),(5,1,'skip-excluded',3),(3,-1,'skip-excluded',5),(3,-1,'all-source',2),(1,1,'all-source',2)]:
            item.mapping['track_mapping']['1']['channel_idx']=[before]
            self.assertEqual(self.plan(direction,scope)['items'][0]['after'],after)
        self.assertEqual(json.dumps(item.source_mapping),source)
    def test_independent_instances_and_video_are_planned_without_writes(self):
        a,b,video=Clip('a',3,5),Clip('b',5,5),Clip('video',kind='video');self.selected=[a,b,video]
        plan=self.plan();self.assertEqual([i['after'] for i in plan['items']],[4,1])
        self.assertEqual((plan['changed'],plan['video_selected'],plan['project_id'],plan['timeline_id']),(0,1,'project','timeline'))
    def test_ambiguous_identity_is_rejected_before_planning(self):
        self.selected=[Clip('a'),Clip('a')]
        self.assertEqual(self.plan()['reason'],'ambiguous-audio-selection')
    def test_no_selection_and_wrong_page(self):
        self.assertEqual(self.plan()['reason'],'no-selected-audio-clips')
        self.resolve.GetCurrentPage=lambda:'deliver'
        self.assertEqual(self.plan()['reason'],'edit-page-required')
    def test_offline_and_single_channel(self):
        self.selected=[Clip('a',1,1)]
        self.assertEqual(self.plan()['items'][0]['reason'],'single-channel-source')
        self.assertEqual(self.plan(available=lambda path:False)['items'][0]['reason'],'source-file-unavailable')
    def test_unsupported_mapping_does_not_guess(self):
        item=Clip('a');self.selected=[item]
        for change,reason in [({'linked_audio':{'x':1}},'linked-source-mapping-unsupported'),({'track_mapping':{'1':{'channel_idx':[1,2],'type':'stereo'}}},'non-mono-mapping-unsupported')]:
            item.mapping.update(change)
            self.assertEqual(self.plan()['items'][0]['reason'],reason)
            item.mapping['linked_audio']={}
    def test_fresh_source_configuration_and_empty_eligibility(self):
        item=Clip('a',3,5);self.selected=[item]
        for channel,reason in [(4,None),(3,'single-eligible-channel'),(0,'no-configured-channels')]:
            item.source_mapping['track_mapping']={'1':{'channel_idx':[channel],'type':'mono','mute':False}}
            entry=self.plan(scope='skip-excluded')['items'][0]
            if reason:self.assertEqual(entry['reason'],reason)
            else:self.assertEqual(entry['after'],channel)
    def test_invalid_direction_and_scope(self):
        self.assertEqual(self.plan(0)['reason'],'invalid-direction')
        self.assertEqual(self.plan(scope='guess')['reason'],'invalid-channel-scope')

if __name__=='__main__':unittest.main()
