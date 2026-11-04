#!/usr/bin/env python3
"""Namespaced metadata experiment; caller must own a disposable project."""
import json

KEY = 'com.cinewaltz.opengate.clip-color-probe.v1'

def write_probe(item, timeline=None):
    kind = 'timeline' if timeline is not None else 'source'
    media = timeline.GetMediaPoolItem() if timeline is not None else item.GetMediaPoolItem()
    value = {'schema': 1, 'label': 'Bright Red', 'rgb': '#FF0000',
             'native_fallback': 'Orange', 'timeline_item_id': item.GetUniqueId()}
    if timeline is not None:
        value['timeline_id'] = timeline.GetUniqueId()
    payload = json.dumps(value, sort_keys=True, separators=(',', ':'))
    result = {'key': KEY, 'payload': payload, 'carrier': kind,
              'accepted': False, 'immediate_matches': False,
              'scope': kind+' MediaPoolItem metadata; instance portability unverified',
              'rendering_verified': False, 'drp_import_verified': False}
    if media is None:
        result['reason'] = 'Requested metadata carrier is unavailable; no alternate object was written'
        return result
    # One additive namespaced key; never replace unrelated third-party metadata.
    result['accepted'] = bool(media.SetThirdPartyMetadata({KEY: payload}))
    result['immediate_matches'] = media.GetThirdPartyMetadata(KEY) == payload
    return result

def read_after_reopen(item, result, timeline=None):
    media = (timeline.GetMediaPoolItem() if timeline is not None else None) if result.get('carrier') == 'timeline' else item.GetMediaPoolItem()
    result['reopened_matches'] = bool(media and media.GetThirdPartyMetadata(KEY) == result['payload'])
    return result
