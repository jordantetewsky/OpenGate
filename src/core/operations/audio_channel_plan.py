"""Read-only selection and source-channel planning shared by audio executors."""
import json
import os


def _channel_change(raw, direction, scope="all-source", source_raw=None):
    mapping = json.loads(raw)
    count = mapping.get("embedded_audio_channels")
    tracks = mapping.get("track_mapping")
    if type(count) is not int or count < 1:
        return None, "no-embedded-audio"
    if mapping.get("linked_audio"):
        return None, "linked-source-mapping-unsupported"
    if not isinstance(tracks, dict) or len(tracks) != 1:
        return None, "multiple-output-mapping-unsupported"
    track = next(iter(tracks.values()))
    if not isinstance(track, dict) or str(track.get("type", "")).lower() != "mono":
        return None, "non-mono-mapping-unsupported"
    channels = track.get("channel_idx")
    if (not isinstance(channels, list) or len(channels) != 1
            or type(channels[0]) is not int or not 1 <= channels[0] <= count):
        return None, "unconnected-or-invalid-channel"
    if type(track.get("mute")) is not bool:
        return None, "invalid-mute-state"
    if count == 1:
        return None, "single-channel-source"
    eligible = list(range(1, count + 1))
    if scope == "skip-excluded":
        if not source_raw:
            return None, "source-configuration-unavailable"
        source = json.loads(source_raw)
        source_tracks = source.get("track_mapping")
        if (source.get("embedded_audio_channels") != count or source.get("linked_audio")
                or not isinstance(source_tracks, dict) or not source_tracks):
            return None, "source-configuration-unsupported"
        configured = set()
        for row in source_tracks.values():
            indices = row.get("channel_idx") if isinstance(row, dict) else None
            if (not isinstance(row, dict) or str(row.get("type", "")).lower() != "mono"
                    or not isinstance(indices, list) or len(indices) != 1
                    or type(indices[0]) is not int or not 0 <= indices[0] <= count):
                return None, "source-configuration-unsupported"
            if indices[0]:
                configured.add(indices[0])
        eligible = sorted(configured)
        if not eligible:
            return None, "no-configured-channels"
    before = channels[0]
    candidates = [c for c in eligible if c > before] if direction > 0 else [c for c in eligible if c < before]
    after = (candidates[0] if candidates else eligible[0]) if direction > 0 else (candidates[-1] if candidates else eligible[-1])
    if after == before:
        return None, "single-eligible-channel"
    track["channel_idx"] = [after]
    return ({"track_mapping": tracks}, before, after), None


def selected_audio_context(resolve, direction, scope):
    if type(direction) is not int or direction not in (-1, 1):
        return None, 0, None, {"success": False, "reason": "invalid-direction"}
    if scope not in ("all-source", "skip-excluded"):
        return None, 0, None, {"success": False, "reason": "invalid-channel-scope"}
    if resolve.GetCurrentPage() != "edit":
        return None, 0, None, {"success": False, "reason": "edit-page-required"}
    manager = resolve.GetProjectManager()
    project = manager.GetCurrentProject() if manager else None
    timeline = project.GetCurrentTimeline() if project else None
    if timeline is None:
        return None, 0, None, {"success": False, "reason": "no-current-timeline"}
    selected = list(timeline.GetSelectedClips() or [])
    seen = set()
    video_count = 0
    for item in selected:
        track_type = item.GetTrackTypeAndIndex()[0]
        if track_type == "video":
            video_count += 1
        if track_type != "audio":
            continue
        identity = item.GetUniqueId()
        if not identity or identity in seen:
            return None, 0, None, {"success": False, "reason": "ambiguous-audio-selection"}
        seen.add(identity)
    return selected, video_count, {"project": project, "timeline": timeline}, None


def plan_audio_item(item, direction, file_available, scope):
    """Read one selected instance; never call a media or timeline setter."""
    entry = {"status": "skipped", "item_id": item.GetUniqueId()}
    try:
        media = item.GetMediaPoolItem()
        path = media.GetClipProperty("File Path") if media else None
        if not isinstance(path, str) or not path or not file_available(path):
            entry["reason"] = "source-file-unavailable"
        else:
            source_raw = media.GetAudioMapping() if scope == "skip-excluded" else None
            raw = item.GetSourceAudioChannelMapping()
            change, reason = _channel_change(raw, direction, scope, source_raw)
            if reason:
                entry["reason"] = reason
            else:
                desired, before, after = change
                entry.update(status="ready", before=before, after=after,
                             source_channels=json.loads(raw)["embedded_audio_channels"],
                             original_mapping=json.loads(raw)["track_mapping"],
                             desired_mapping=desired["track_mapping"])
    except Exception as error:
        entry.update(status="failed", reason="audio-mapping-error", error_type=type(error).__name__)
    return entry


def plan_selected_audio(resolve, direction, file_available=os.path.isfile, scope="all-source"):
    """Return a serializable plan, not a completed operation or an Undo claim.

    Consumers must revalidate context/mapping before committing. This snapshot
    grants no authority to apply a plan later to a newly selected clip.
    """
    selected, video_count, context, error = selected_audio_context(resolve, direction, scope)
    if error:
        return error
    try:
        project_id = context["project"].GetUniqueId()
        timeline_id = context["timeline"].GetUniqueId()
        if not project_id or not timeline_id:
            raise ValueError("missing native context identity")
    except (AttributeError, TypeError, ValueError):
        return {"success": False, "reason": "native-context-identity-unavailable"}
    items = [plan_audio_item(item, direction, file_available, scope)
             for item in selected if item.GetTrackTypeAndIndex()[0] == "audio"]
    ready = sum(item["status"] == "ready" for item in items)
    return {"success": ready > 0 and not any(i["status"] == "failed" for i in items),
            "phase": "plan-only", "project_id": project_id, "timeline_id": timeline_id, "changed": 0, "ready": ready, "items": items,
            "audio_selected": len(items), "video_selected": video_count,
            "channel_scope": scope, "direction": direction,
            "reason": "" if ready else ("no-selected-audio-clips" if not items else "no-clips-planned")}
