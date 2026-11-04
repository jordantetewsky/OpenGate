#!/usr/bin/env python3
"""Read-only verification of the installed SDK's Timeline selection identity."""

def inspect_selection(resolve):
    result = {'scope': 'documented SDK selection reads only', 'project_writes': False,
              'selection_supported': False, 'identity_consistent': False,
              'native_graphics_join_verified': False, 'native_undo_verified': False}
    manager = resolve.GetProjectManager()
    project = manager.GetCurrentProject()
    if not project:
        result['reason'] = 'No project is open'
        return result
    timeline = project.GetCurrentTimeline()
    if not timeline:
        result['reason'] = 'No current Timeline'
        return result
    try:
        result['project_id'] = project.GetUniqueId()
        result['timeline_id'] = timeline.GetUniqueId()
        selected = timeline.GetSelectedClips()
        if not isinstance(selected, list):
            result['reason'] = 'GetSelectedClips did not return the documented list'
            return result
        result['selection_supported'] = True
        result['clips'] = []
        for clip in selected:
            media = clip.GetMediaPoolItem()
            result['clips'].append({'id': clip.GetUniqueId(),
                'source_id': media.GetUniqueId() if media else None,
                'start': clip.GetStart(), 'end': clip.GetEnd(),
                'native_color': clip.GetClipColor()})
        after = timeline.GetSelectedClips()
        after_ids = [clip.GetUniqueId() for clip in after] if isinstance(after, list) else None
        ids = [clip['id'] for clip in result['clips']]
        current = manager.GetCurrentProject()
        current_timeline = current.GetCurrentTimeline() if current else None
        result['identity_consistent'] = bool(
            current and current_timeline and current.GetUniqueId() == result['project_id']
            and current_timeline.GetUniqueId() == result['timeline_id']
            and after_ids is not None and sorted(ids) == sorted(after_ids)
            and len(set(ids)) == len(ids) and all(isinstance(value, str) and value for value in ids))
        result['reason'] = 'Read-only selection inspected; no assignment or portability claim'
    except Exception as error:
        result['reason'] = 'Selection inspection failed: ' + str(error)
    return result
