#!/usr/bin/env python3
"""Reimport only a caller-owned scratch DRP; never switch an editing project."""
import uuid
from clipcolorprobescope import ScratchProjectScope, empty_placeholder


def inspect_import(manager, archive, expected, key, payload, observer=None, carrier='source', allow_empty_placeholder=False):
    if carrier not in ('source','timeline'):
        raise ValueError('Unknown metadata carrier')
    result = {'attempted': False, 'metadata_survives': False,
              'instance_identity_survives': False, 'native_undo_verified': False}
    active = manager.GetCurrentProject()
    if active and not (allow_empty_placeholder and empty_placeholder(active)):
        raise RuntimeError('DRP import requires Project Manager with no project open')
    name = 'OpenGate Color Import Probe ' + uuid.uuid4().hex[:10]
    scope = ScratchProjectScope(manager,name,allow_empty_placeholder)
    result['disposable_project'] = name
    def publish():
        if observer:
            observer(dict(result))
    publish()
    try:
        if not manager.ImportProject(str(archive), name):
            raise RuntimeError('Cannot import the scratch DRP')
        result['attempted'] = True
        publish()
        current = manager.GetCurrentProject()
        if current and current.GetName() == name:
            scope.bind(current)
        else:
            scope.require_empty()
            scope.bind(manager.LoadProject(name))
        timeline = scope.project.GetCurrentTimeline()
        items = timeline.GetItemListInTrack('audio', 1)
        if len(items) != 1:
            raise RuntimeError('Imported scratch timeline changed clip count')
        clip = items[0]
        source = clip.GetMediaPoolItem()
        observed = {'timeline_id': timeline.GetUniqueId(), 'id': clip.GetUniqueId(),
                    'media_id': source.GetUniqueId(), 'start': clip.GetStart(),
                    'end': clip.GetEnd(), 'duration': clip.GetDuration()}
        result['observed'] = observed
        metadata = timeline.GetMediaPoolItem() if carrier == 'timeline' else source
        result['metadata_carrier'] = carrier
        result['metadata_survives'] = bool(metadata and metadata.GetThirdPartyMetadata(key) == payload)
        result['timeline_identity_survives'] = bool(expected['timeline_id']) and observed['timeline_id'] == expected['timeline_id']
        result['instance_identity_survives'] = bool(expected['id']) and observed['id'] == expected['id']
        result['source_identity_survives'] = bool(expected['media_id']) and observed['media_id'] == expected['media_id']
        result['structure_unchanged'] = all(observed[k] == expected[k] for k in ('start', 'end', 'duration'))
        result['native_control_color_survives'] = clip.GetClipColor() == 'Orange'
        result['instance_association_portable'] = all(result[k] for k in
            ('metadata_survives', 'timeline_identity_survives', 'instance_identity_survives',
             'source_identity_survives', 'structure_unchanged', 'native_control_color_survives'))
    finally:
        if scope.project:
            result['closed_disposable_project'] = bool(scope.cleanup())
            if not result['closed_disposable_project']:
                result['cleanup_skipped_or_failed'] = True
        publish()
    return result
