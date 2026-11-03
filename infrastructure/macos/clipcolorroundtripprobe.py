#!/usr/bin/env python3
"""Reimport only a caller-owned scratch DRP; never switch an editing project."""
import uuid


def inspect_import(manager, archive, expected, key, payload, observer=None):
    result = {'attempted': False, 'metadata_survives': False,
              'instance_identity_survives': False, 'native_undo_verified': False}
    if manager.GetCurrentProject():
        raise RuntimeError('DRP import requires Project Manager with no project open')
    name = 'OpenGate Color Import Probe ' + uuid.uuid4().hex[:10]
    result['disposable_project'] = name
    def publish():
        if observer:
            observer(dict(result))
    publish()
    project = None
    try:
        if not manager.ImportProject(str(archive), name):
            raise RuntimeError('Cannot import the scratch DRP')
        result['attempted'] = True
        publish()
        if manager.GetCurrentProject():
            raise RuntimeError('A project opened during import; refusing to switch it')
        project = manager.LoadProject(name)
        if not project:
            raise RuntimeError('Cannot open the imported scratch project')
        timeline = project.GetCurrentTimeline()
        items = timeline.GetItemListInTrack('audio', 1)
        if len(items) != 1:
            raise RuntimeError('Imported scratch timeline changed clip count')
        clip = items[0]
        source = clip.GetMediaPoolItem()
        observed = {'timeline_id': timeline.GetUniqueId(), 'id': clip.GetUniqueId(),
                    'media_id': source.GetUniqueId(), 'start': clip.GetStart(),
                    'end': clip.GetEnd(), 'duration': clip.GetDuration()}
        result['observed'] = observed
        result['metadata_survives'] = source.GetThirdPartyMetadata(key) == payload
        result['timeline_identity_survives'] = bool(expected['timeline_id']) and observed['timeline_id'] == expected['timeline_id']
        result['instance_identity_survives'] = bool(expected['id']) and observed['id'] == expected['id']
        result['source_identity_survives'] = bool(expected['media_id']) and observed['media_id'] == expected['media_id']
        result['structure_unchanged'] = all(observed[k] == expected[k] for k in ('start', 'end', 'duration'))
        result['native_control_color_survives'] = clip.GetClipColor() == 'Orange'
        result['instance_association_portable'] = all(result[k] for k in
            ('metadata_survives', 'timeline_identity_survives', 'instance_identity_survives',
             'source_identity_survives', 'structure_unchanged', 'native_control_color_survives'))
    finally:
        if project:
            result['closed_disposable_project'] = bool(manager.CloseProject(project))
        publish()
    return result
