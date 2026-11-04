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

def run(resolve, output, execute=False, metadata=False, roundtrip=False, selection=False, carrier='source', allow_empty_placeholder=False):
    output = output.resolve()
    manager = resolve.GetProjectManager()
    receipt = {'schema': 1, 'component': 'OpenGate', 'experiment': 'selected-clip-identity' if selection else 'clip-color-metadata' if metadata else 'red-clip-color',
               'time': datetime.datetime.now(datetime.timezone.utc).isoformat(),
               'mutated_existing_project': False, 'status': 'not-run'}
    output.mkdir(parents=True, exist_ok=True)
    def record():
        pending = output/'receipt.pending'
        pending.write_text(json.dumps(receipt, indent=2)+'\n')
        pending.replace(output/'receipt.json')
    if selection:
        if execute:
            receipt['status'] = 'failed'; receipt['reason'] = 'Selection inspection cannot be combined with writes'
        else:
            from clipcolorselectionprobe import inspect_selection
            try:
                receipt['selection'] = inspect_selection(resolve)
                receipt['status'] = 'inspected'
            except Exception as error:
                receipt['status'] = 'failed'; receipt['reason'] = 'Read-only selection inspection failed: '+str(error)
        record(); return receipt
    if not execute:
        receipt['reason'] = 'Read-only connectivity check; use --run with no project open.'
        receipt['project_open'] = bool(manager.GetCurrentProject()); record(); return receipt
    from clipcolorprobescope import ScratchProjectScope, empty_placeholder
    active = manager.GetCurrentProject()
    if active and not (allow_empty_placeholder and empty_placeholder(active)):
        receipt['reason'] = 'An existing project is open. Refusing to switch, save or mutate it.'
        record(); return receipt
    name = 'OpenGate Red Storage Probe '+uuid.uuid4().hex[:10]
    receipt['disposable_project'] = name
    receipt['status'] = 'running'; record()
    receipt['empty_project_manager_placeholder'] = bool(active)
    scope = ScratchProjectScope(manager,name,allow_empty_placeholder)
    try:
        # Only generated silence is imported; no user media or editing project.
        media = output/'generated-silence.wav'
        with wave.open(str(media),'wb') as audio:
            audio.setnchannels(1); audio.setsampwidth(2); audio.setframerate(48000)
            audio.writeframes(b'\0\0' * 48000 * 3)
        scope.require_empty();scope.bind(manager.CreateProject(name))
        pool = scope.project.GetMediaPool(); clips = pool.ImportMedia([str(media)])
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
            scope.require_current()
            result = write_probe(item,timeline if carrier=='timeline' else None); receipt['metadata'] = result; record()
            if not scope.save(): raise RuntimeError('Cannot save disposable metadata project')
            if not scope.close(): raise RuntimeError('Cannot close disposable metadata project')
            scope.require_empty();scope.bind(manager.LoadProject(name))
            items = scope.project.GetCurrentTimeline().GetItemListInTrack('audio',1)
            if len(items) != 1: raise RuntimeError('Metadata probe changed clip count')
            item = items[0]; read_after_reopen(item,result,scope.project.GetCurrentTimeline() if carrier=='timeline' else None)
            result['structure_unchanged_after_reopen'] = snapshot(item) == baseline
            result['storage_verified'] = all(result[key] for key in
                ('accepted','immediate_matches','reopened_matches','structure_unchanged_after_reopen'))
            record()
        for color in ([] if metadata else ['Red', '#FF0000', 'red']):
            scope.require_current()
            if not item.SetClipColor('Orange'): raise RuntimeError('Native control color failed')
            accepted = bool(item.SetClipColor(color))
            result = {'requested': color, 'accepted': accepted, 'immediate': item.GetClipColor(),
                      'structure_unchanged': snapshot(item) == baseline}
            if not scope.save(): raise RuntimeError('Cannot save disposable project')
            if not scope.close(): raise RuntimeError('Cannot close disposable project')
            scope.require_empty();scope.bind(manager.LoadProject(name))
            timeline = scope.project.GetCurrentTimeline(); items = timeline.GetItemListInTrack('audio',1)
            if len(items) != 1: raise RuntimeError('Clip count changed')
            item = items[0]; result['reopened'] = item.GetClipColor()
            result['structure_unchanged_after_reopen'] = snapshot(item) == baseline
            receipt['attempts'].append(result); record()
        # Leave a supported native color on the real clip, independent of Unbound.
        scope.require_current()
        if not item.SetClipColor('Orange') or not scope.save(): raise RuntimeError('Cannot restore native control color')
        archive = output/'disposable-project.drp'
        receipt['exported'] = bool(manager.ExportProject(name,str(archive)))
        if roundtrip:
            if not metadata or not receipt['exported']:
                raise RuntimeError('DRP round trip requires a successful metadata scratch export')
            expected = snapshot(item)
            expected['timeline_id'] = scope.project.GetCurrentTimeline().GetUniqueId()
            if not scope.close():
                raise RuntimeError('Cannot close scratch project before DRP import')
            receipt['drp_import_requested'] = True; record()
            from clipcolorroundtripprobe import inspect_import
            def import_observation(value):
                receipt['drp_roundtrip'] = value
                record()
            receipt['drp_roundtrip'] = inspect_import(manager, archive, expected,
                receipt['metadata']['key'], receipt['metadata']['payload'], import_observation, carrier,allow_empty_placeholder)
            receipt['metadata']['drp_import_verified'] = receipt['drp_roundtrip']['metadata_survives']
            receipt['metadata']['instance_association_portable'] = receipt['drp_roundtrip']['instance_association_portable']
        receipt['status'] = 'completed'
        receipt['red_storage_verified'] = any(x['accepted'] and x['immediate']=='Red' and x['reopened']=='Red'
            and x['structure_unchanged_after_reopen'] for x in receipt['attempts'])
    except Exception as error:
        receipt['status'] = 'failed'; receipt['error'] = str(error)
    finally:
        if scope.project:
            receipt['closed_disposable_project'] = bool(scope.cleanup())
            if not receipt['closed_disposable_project']: receipt['cleanup_skipped_or_failed'] = True
        record()
    return receipt

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--run', action='store_true')
    parser.add_argument('--metadata', action='store_true', help='Probe namespaced hue metadata instead of native Red values')
    parser.add_argument('--drp-roundtrip', action='store_true', help='Reimport only the generated scratch DRP and compare IDs/metadata; requires --metadata')
    parser.add_argument('--inspect-selection', action='store_true', help='Read selected Timeline IDs/colors only; never creates, saves or switches a project')
    parser.add_argument('--metadata-carrier',choices=('source','timeline'),default='source',help='Choose the scratch metadata carrier; requires --metadata')
    parser.add_argument('--allow-empty-placeholder',action='store_true',help='After user confirms fresh Project Manager, allow its verified empty Untitled Project placeholder')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--worker', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.drp_roundtrip and not args.metadata:
        parser.error('--drp-roundtrip requires --metadata')
    if args.inspect_selection and (args.run or args.metadata or args.drp_roundtrip):
        parser.error('--inspect-selection is read-only and cannot be combined with write probes')
    if args.metadata_carrier!='source' and not args.metadata:
        parser.error('--metadata-carrier timeline requires --metadata')
    if not args.worker:
        try:
            completed = subprocess.run([sys.executable, __file__, '--worker', '--output', str(args.output),'--metadata-carrier',args.metadata_carrier] + (['--run'] if args.run else []) + (['--metadata'] if args.metadata else []) + (['--drp-roundtrip'] if args.drp_roundtrip else []) + (['--inspect-selection'] if args.inspect_selection else []) + (['--allow-empty-placeholder'] if args.allow_empty_placeholder else []), timeout=30)
            sys.exit(completed.returncode)
        except subprocess.TimeoutExpired:
            args.output.mkdir(parents=True, exist_ok=True)
            (args.output/'timeout.json').write_text(json.dumps({'component':'OpenGate','experiment':'selected-clip-identity' if args.inspect_selection else 'clip-color-metadata' if args.metadata else 'red-clip-color','status':'timeout','storage_verified':False})+'\n')
            sys.exit('Color probe timed out; no storage success claimed. See timeout.json.')
    sys.path.insert(0, MODULES)
    import DaVinciResolveScript
    resolve = DaVinciResolveScript.scriptapp('Resolve')
    if not resolve:
        args.output.mkdir(parents=True, exist_ok=True)
        pending=args.output/'receipt.pending'
        pending.write_text(json.dumps({'schema':1,'component':'OpenGate',
            'experiment':'selected-clip-identity' if args.inspect_selection else 'clip-color-metadata' if args.metadata else 'red-clip-color',
            'status':'unavailable','reason':'Resolve scripting is unavailable',
            'mutated_existing_project':False,'storage_verified':False})+'\n')
        pending.replace(args.output/'receipt.json')
        sys.exit('Resolve scripting is unavailable; receipt retained in '+str(args.output))
    result = run(resolve,args.output,args.run,args.metadata,args.drp_roundtrip,args.inspect_selection,args.metadata_carrier,args.allow_empty_placeholder)
    print(json.dumps(result,indent=2))
    sys.exit(1 if result['status']=='failed' else 0)
