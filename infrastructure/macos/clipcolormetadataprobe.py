#!/usr/bin/env python3
"""Namespaced metadata experiment; caller must own a disposable project."""
import json

KEY = 'com.cinewaltz.opengate.clip-color-probe.v1'

def write_probe(item):
    media = item.GetMediaPoolItem()
    payload = json.dumps({'schema': 1, 'label': 'Bright Red', 'rgb': '#FF0000',
                          'native_fallback': 'Orange', 'timeline_item_id': item.GetUniqueId()},
                         sort_keys=True, separators=(',', ':'))
    # One additive namespaced key; never replace unrelated third-party metadata.
    accepted = bool(media.SetThirdPartyMetadata({KEY: payload}))
    return {'key': KEY, 'payload': payload, 'accepted': accepted,
            'immediate_matches': media.GetThirdPartyMetadata(KEY) == payload,
            'scope': 'MediaPoolItem metadata; timeline-instance portability unverified',
            'rendering_verified': False, 'drp_import_verified': False}

def read_after_reopen(item, result):
    result['reopened_matches'] = item.GetMediaPoolItem().GetThirdPartyMetadata(KEY) == result['payload']
    return result
