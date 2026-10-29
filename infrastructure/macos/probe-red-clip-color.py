#!/usr/bin/env python3
"""Disposable-project storage probe. Never modifies an already-open project."""
import argparse
import datetime
import json
from pathlib import Path
import sys
import subprocess
import uuid
import wave

MODULES = '/Library/Application Support/Blackmagic Design/DaVinci Resolve/Developer/Scripting/Modules'

def run(resolve, output, execute=False, metadata=False):
    manager = resolve.GetProjectManager()
    receipt = {'schema': 1, 'component': 'OpenGate', 'experiment': 'clip-color-metadata' if metadata else 'red-clip-color',
               'time': datetime.datetime.now(datetime.timezone.utc).isoformat(),
               'mutated_existing_project': False, 'status': 'not-run'}
    output.mkdir(parents=True, exist_ok=True)
    def record():
        (output/'receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
    if not execute:
        receipt['reason'] = 'Read-only connectivity check; use --run with no project open.'
        receipt['project_open'] = bool(manager.GetCurrentProject()); record(); return receipt
    if manager.GetCurrentProject():
        receipt['reason'] = 'An existing project is open. Refusing to switch, save or mutate it.'
        record(); return receipt
    name = 'OpenGate Red Storage Probe '+uuid.uuid4().hex[:10]
    receipt['disposable_project'] = name
    receipt['status'] = 'running'; record()
    project = None
    try:
        # Only generated silence is imported; no user media or editing project.
        media = output/'generated-silence.wav'
        with wave.open(str(media),'wb') as audio:
            audio.setnchannels(1); audio.setsampwidth(2); audio.setframerate(48000)
            audio.writeframes(b'\0\0' * 48000 * 3)
        project = manager.CreateProject(name)
        if not project: raise RuntimeError('Cannot create disposable project')
        pool = project.GetMediaPool(); clips = pool.ImportMedia([str(media)])
        if not clips: raise RuntimeError('Cannot import generated test media')
        timeline = pool.CreateTimelineFromClips('Clip Color Test', clips)
        if not timeline: raise RuntimeError('Cannot create test timeline')
        items = timeline.GetItemListInTrack('audio',1)
        if len(items) != 1: raise RuntimeError('Expected one generated audio clip')
        item = items[0]
        def snapshot(clip):
            return {'id': clip.GetUniqueId(), 'start': clip.GetStart(), 'end': clip.GetEnd(),
                    'duration': clip.GetDuration(), 'media_id': clip.GetMediaPoolItem().GetUniqueId()}
        baseline = snapshot(item); receipt['attempts'] = []
        if metadata:
            from clipcolormetadataprobe import write_probe, read_after_reopen
            result = write_probe(item); receipt['metadata'] = result; record()
            if not manager.SaveProject(): raise RuntimeError('Cannot save disposable metadata project')
            if not manager.CloseProject(project): raise RuntimeError('Cannot close disposable metadata project')
            project = None
            project = manager.LoadProject(name)
            if not project: raise RuntimeError('Cannot reopen disposable metadata project')
            items = project.GetCurrentTimeline().GetItemListInTrack('audio',1)
            if len(items) != 1: raise RuntimeError('Metadata probe changed clip count')
            item = items[0]; read_after_reopen(item,result)
            result['structure_unchanged_after_reopen'] = snapshot(item) == baseline
            result['storage_verified'] = all(result[key] for key in
                ('accepted','immediate_matches','reopened_matches','structure_unchanged_after_reopen'))
            record()
        for color in ([] if metadata else ['Red', '#FF0000', 'red']):
            if not item.SetClipColor('Orange'): raise RuntimeError('Native control color failed')
            accepted = bool(item.SetClipColor(color))
            result = {'requested': color, 'accepted': accepted, 'immediate': item.GetClipColor(),
                      'structure_unchanged': snapshot(item) == baseline}
            if not manager.SaveProject(): raise RuntimeError('Cannot save disposable project')
            if not manager.CloseProject(project): raise RuntimeError('Cannot close disposable project')
            project = manager.LoadProject(name)
            if not project: raise RuntimeError('Cannot reopen disposable project')
            timeline = project.GetCurrentTimeline(); items = timeline.GetItemListInTrack('audio',1)
            if len(items) != 1: raise RuntimeError('Clip count changed')
            item = items[0]; result['reopened'] = item.GetClipColor()
            result['structure_unchanged_after_reopen'] = snapshot(item) == baseline
            receipt['attempts'].append(result); record()
        # Leave a supported native color on the real clip, independent of Unbound.
        if not item.SetClipColor('Orange') or not manager.SaveProject(): raise RuntimeError('Cannot restore native control color')
        receipt['exported'] = bool(manager.ExportProject(name,str(output/'disposable-project.drp')))
        receipt['status'] = 'completed'
        receipt['red_storage_verified'] = any(x['accepted'] and x['immediate']=='Red' and x['reopened']=='Red'
            and x['structure_unchanged_after_reopen'] for x in receipt['attempts'])
    except Exception as error:
        receipt['status'] = 'failed'; receipt['error'] = str(error)
    finally:
        if project: receipt['closed_disposable_project'] = bool(manager.CloseProject(project))
        record()
    return receipt

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', action='store_true')
    parser.add_argument('--metadata', action='store_true', help='Probe namespaced hue metadata instead of native Red values')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if not args.worker:
        try:
            completed = subprocess.run([sys.executable, __file__, '--worker', '--output', str(args.output)] + (['--run'] if args.run else []) + (['--metadata'] if args.metadata else []), timeout=30)
            sys.exit(completed.returncode)
        except subprocess.TimeoutExpired:
            args.output.mkdir(parents=True, exist_ok=True)
            (args.output/'timeout.json').write_text(json.dumps({'component':'OpenGate','experiment':'clip-color-metadata' if args.metadata else 'red-clip-color','status':'timeout','storage_verified':False})+'\n')
            sys.exit('Color probe timed out; no storage success claimed. See timeout.json.')
    sys.path.insert(0, MODULES)
    import DaVinciResolveScript
    resolve = DaVinciResolveScript.scriptapp('Resolve')
    if not resolve: sys.exit('Resolve scripting is unavailable')
    result = run(resolve,args.output,args.run,args.metadata)
    print(json.dumps(result,indent=2))
    sys.exit(1 if result['status']=='failed' else 0)
