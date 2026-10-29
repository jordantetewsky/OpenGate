import importlib.util
from pathlib import Path
from types import SimpleNamespace as Obj
import unittest

path = Path(__file__).resolve().parents[3] / "infrastructure/macos/selected-clip-stabilization.py"
spec = importlib.util.spec_from_file_location("stabilization", path)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class Clip:
    def __init__(self, kind="video", success=True):
        self.kind, self.success, self.calls = kind, success, 0

    def GetTrackTypeAndIndex(self):
        return [self.kind, 1]

    def Stabilize(self):
        self.calls += 1
        return self.success


def resolve(clips, page="edit"):
    timeline = Obj(GetSelectedClips=lambda: clips)
    project = Obj(GetCurrentTimeline=lambda: timeline)
    return Obj(GetCurrentPage=lambda: page,
               GetProjectManager=lambda: Obj(GetCurrentProject=lambda: project))


class SelectionTests(unittest.TestCase):
    def test_no_widgets_required_and_every_selected_video_runs_once(self):
        clips = [Clip(), Clip("audio"), Clip()]
        result = module.stabilize_selected(resolve(clips))
        self.assertTrue(result["success"])
        self.assertEqual([c.calls for c in clips], [1, 0, 1])

    def test_no_selection_does_not_substitute_playhead(self):
        self.assertEqual(module.stabilize_selected(resolve([]))["reason"],
                         "no-selected-video-clips")

    def test_cut_does_not_execute(self):
        clip = Clip()
        self.assertFalse(module.stabilize_selected(resolve([clip], "cut"))["success"])
        self.assertEqual(clip.calls, 0)

    def test_api_failure_is_not_reported_as_success(self):
        result = module.stabilize_selected(resolve([Clip(success=False)]))
        self.assertFalse(result["success"])
        self.assertEqual(result["completed"], 0)

    def test_audio_selection_does_not_stabilize_playhead_video(self):
        audio = Clip("audio")
        self.assertFalse(module.stabilize_selected(resolve([audio]))["success"])
        self.assertEqual(audio.calls, 0)

    def test_one_failure_does_not_hide_another_selected_result(self):
        clips = [Clip(success=False), Clip()]
        result = module.stabilize_selected(resolve(clips))
        self.assertFalse(result["success"])
        self.assertEqual((result["attempted"], result["completed"]), (2, 1))

    def test_missing_timeline_is_explicit(self):
        api = Obj(GetCurrentPage=lambda: "edit", GetProjectManager=lambda: None)
        self.assertEqual(module.stabilize_selected(api)["reason"], "no-current-timeline")


if __name__ == "__main__":
    unittest.main()
