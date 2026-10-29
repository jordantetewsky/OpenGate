"""Generate direct Resolve API wrappers with one completion transport."""


def build_operation_wrapper(command, marker, operation, expression, guard):
    return '''# OpenGate by CineWaltz.
# opengate-command-route: MARKER
import json, os, pathlib, time, uuid
'''.replace('MARKER', marker) + operation + '''
_started_ms=int(time.time()*1000)
try:
    import builtins
    if getattr(builtins,GUARD,False):
        _result={"success":False,"reason":"operation-already-running"}
    else:
        setattr(builtins,GUARD,True)
        try:
            _resolve=globals().get("resolve")
            _result=EXPRESSION if _resolve is not None else {"success":False,"reason":"resolve-context-unavailable"}
        finally:
            setattr(builtins,GUARD,False)
except Exception as _error:
    _result={"success":False,"reason":"scripting-api-error","error_type":type(_error).__name__}
_result.update({"command":COMMAND,"issued_ms":int(time.time()*1000),"started_ms":_started_ms})
base=pathlib.Path(os.environ.get("UNBOUND_LAUNCH_DIRECTORY",str(pathlib.Path.home()/"Library/Logs/OpenGate")))
directory=base/"command-results"
directory.mkdir(parents=True,exist_ok=True)
token=str(uuid.uuid4())
temporary=directory/(token+".tmp")
temporary.write_text(json.dumps(_result))
os.replace(temporary,directory/(token+".json"))
'''.replace('GUARD', repr(guard)).replace('EXPRESSION', expression).replace('COMMAND', repr(command))
