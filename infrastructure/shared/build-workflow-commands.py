#!/usr/bin/env python3
"""Build individual Resolve Workflow Integration shortcut targets (no install)."""
import json
import runpy
import argparse
from pathlib import Path
root=Path(__file__).resolve().parents[2]
build_operation_wrapper=runpy.run_path(str(Path(__file__).with_name('workflow_operation_wrapper.py')))['build_operation_wrapper']
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--platform',choices=('macos','windows'),default='macos')
parser.add_argument('--output',type=Path)
args=parser.parse_args()
output=args.output or root/('build/'+args.platform+'/workflow-commands')
output.mkdir(parents=True,exist_ok=True)
panels=[('frameConformTimelines','Timeline'),('widgetEditInspector','Inspector'),('widgetEditSourcePlayer','Source Monitor'),
        ('widgetEditTimelinePlayer','Program Monitor'),('frameEditPageMediaPool','Media Pool'),('frameEditPageEditIndex','Index'),
        ('frameEditPageEffectsLibrary','Effects'),('frameEditPageMetadata','Metadata'),('frameEditAudioMeter','Mixer'),
        ('frameEditPageSoundLibrary','Sound Library'),('frameEditPageKeyframeEditor','Keyframes'),
        ('frameEditToolbarTop','Timeline Toolbar'),('unboundWorkspaceToolbar','Workspace Toolbar')]
commands=[('panel.toggle.'+id,'Show-Hide '+label) for id,label in panels]
commands += [('panel.expand.'+id,'Maximize-Restore '+label) for id,label in panels]
commands += [('panel.expand.hovered','Maximize-Restore Hovered Panel'),('panel.toggle.page-navigation','Show-Hide Page Navigation'),
             ('edit.timeline.fixed-playhead','Edit - Toggle Fixed Playhead'),('edit.inspector.stabilize','Edit - Stabilize'),
             ('edit.audio.previous-channel','Previous Audio Channel — Skip Excluded Channels'),
             ('edit.audio.next-channel','Next Audio Channel — Skip Excluded Channels'),
             ('edit.audio.previous-channel-all-source','Previous Audio Channel — All Source Channels'),
             ('edit.audio.next-channel-all-source','Next Audio Channel — All Source Channels')]
manifest=[]
for command,label in commands:
    filename='OpenGate - '+label+'.py'
    source='''# OpenGate by CineWaltz; native Workflow Integration command bridge.
import json, os, pathlib, time, uuid
base=pathlib.Path(os.environ.get("UNBOUND_LAUNCH_DIRECTORY",str(pathlib.Path.home()/"Library/Logs/OpenGate")))
directory=base/"command-requests"
directory.mkdir(parents=True,exist_ok=True)
token=str(uuid.uuid4())
temporary=directory/(token+".tmp")
temporary.write_text(json.dumps({"command":COMMAND,"issued_ms":int(time.time()*1000)}))
os.replace(temporary,directory/(token+".json"))
# Wake the GUI main queue immediately; directory watching remains a fallback.
try:
    import ctypes
    post=ctypes.CDLL(None).notify_post
    post.argtypes=[ctypes.c_char_p]
    post.restype=ctypes.c_uint32
    post(b"com.cinewaltz.opengate.command-request")
except (ImportError,AttributeError,OSError):
    pass
'''.replace('COMMAND',repr(command))
    if args.platform=='windows':
        start=source.index('    post=ctypes.CDLL(None).notify_post')
        end=source.index('except (ImportError,AttributeError,OSError):',start)
        source=source[:start]+'''    user32=ctypes.WinDLL("user32",use_last_error=True)
    register=user32.RegisterWindowMessageW
    register.argtypes=[ctypes.c_wchar_p]
    register.restype=ctypes.c_uint
    post=user32.PostMessageW
    post.argtypes=[ctypes.c_void_p,ctypes.c_uint,ctypes.c_size_t,ctypes.c_ssize_t]
    post.restype=ctypes.c_int
    message=register("OpenGate.WorkflowCommandReady.v1")
    if message: post(0xffff,message,0,0)
'''+source[end:]
    if command=='edit.inspector.stabilize':
        # Resolve supplies `resolve` in its internal Workflow Integration host.
        # Both menu and native-preset hotkey enter this one operation. Never post
        # another stabilization request to the Qt registry (which would recurse).
        operation=(root/'infrastructure/macos/selected-clip-stabilization.py').read_text()
        source=build_operation_wrapper(command,'selected-timeline-api-v1',operation,
            'stabilize_selected(_resolve)','_opengate_stabilizing')
        source=source.replace('operation-already-running','stabilization-already-running')
    if command.startswith('edit.audio.'):
        operation=(root/'src/core/operations/audio_channel_plan.py').read_text()
        transport=(root/'src/core/operations/audio_channel_native_wrapper.py').read_text()
        direction=-1 if command.startswith('edit.audio.previous-channel') else 1
        scope='all-source' if command.endswith('-all-source') else 'skip-excluded'
        source='# OpenGate by CineWaltz.\n# opengate-command-route: native-audio-plan-v3\n'+operation+'\n'+transport
        source+='\nbase=pathlib.Path(os.environ.get("UNBOUND_LAUNCH_DIRECTORY",str(pathlib.Path.home()/"Library/Logs/OpenGate")))\n'
        source+='dispatch_native_audio_plan(globals().get("resolve"),'+repr(command)+','+str(direction)+','+repr(scope)+',base)\n'
    if args.platform=='windows':
        source=source.replace('str(pathlib.Path.home()/"Library/Logs/OpenGate")',
            'str(pathlib.Path(os.environ.get("LOCALAPPDATA",str(pathlib.Path.home()/"AppData/Local")))/"OpenGate/Logs")')
    (output/filename).write_text(source)
    manifest.append(dict(command=command,label='OpenGate - '+label,file=filename))
(output/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
if args.platform=='windows':
    # Reuse the accepted menu entry, adapting only its platform log directory.
    entry=(root/'src/platforms/macos/OpenGate.py').read_text()
    entry=entry.replace('directory = pathlib.Path.home() / "Library/Logs/OpenGate"',
        'directory = pathlib.Path(os.environ.get("UNBOUND_LAUNCH_DIRECTORY",str(pathlib.Path(os.environ.get("LOCALAPPDATA",str(pathlib.Path.home()/"AppData/Local")))/"OpenGate/Logs")))')
    (output/'OpenGate.py').write_text(entry)
print('Built',len(manifest),'individual OpenGate workflow command targets:',output)
