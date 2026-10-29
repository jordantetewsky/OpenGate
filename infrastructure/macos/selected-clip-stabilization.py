"""Resolve API operation; deliberately independent of Inspector widgets.

The caller supplies Resolve's scripting object. Transport into the running
application is separate: importing this module never changes a project.
"""


def stabilize_selected(resolve):
    if resolve.GetCurrentPage() != "edit":
        return {"success": False, "reason": "edit-page-required"}
    manager = resolve.GetProjectManager()
    project = manager.GetCurrentProject() if manager else None
    timeline = project.GetCurrentTimeline() if project else None
    if timeline is None:
        return {"success": False, "reason": "no-current-timeline"}
    # Snapshot the actual timeline selection; never substitute the media pool
    # selection or a clip under the playhead when nothing is selected.
    selected = list(timeline.GetSelectedClips() or [])
    video = [clip for clip in selected
             if clip.GetTrackTypeAndIndex()[0] == "video"]
    if not video:
        return {"success": False, "reason": "no-selected-video-clips"}
    results = []
    for clip in video:
        try:
            results.append(bool(clip.Stabilize()))
        except Exception:
            results.append(False)
    return {"success": all(results), "attempted": len(results),
            "completed": sum(results), "method": "timeline-item-api"}
