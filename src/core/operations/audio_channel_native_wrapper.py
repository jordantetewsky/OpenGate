# OpenGate native audio planner transport. This file performs no media writes.
import json, os, pathlib, time, uuid

def dispatch_native_audio_plan(resolve, command, direction, scope, base):
    job_path = base / 'audio-channel-plan-job.json'
    try:
        job = json.loads(job_path.read_text())
    except FileNotFoundError:
        job = None
    except (OSError, ValueError):
        return  # A malformed/existing ticket cannot become a fresh command.
    now = int(time.time() * 1000)
    if job is None:
        # Only a true native-menu invocation with no pending/completed ticket
        # enters the registry. A late or different script may never retry.
        directory = base / 'command-requests'
        directory.mkdir(parents=True, exist_ok=True)
        token = str(uuid.uuid4())
        temporary = directory / (token + '.tmp')
        temporary.write_text(json.dumps({'command': command, 'issued_ms': now}))
        os.replace(temporary, directory / (token + '.json'))
        return
    if (not isinstance(job, dict) or job.get('protocol') != 'native-audio-plan-v3'
            or job.get('command') != command or job.get('cancelled') or job.get('inactive')):
        return
    issued = job.get('issued_ms')
    try:
        valid_token = str(uuid.UUID(job.get('token', ''))) == job['token']
    except (ValueError, TypeError, AttributeError, KeyError):
        valid_token = False
    if not valid_token or type(issued) is not int or not 0 <= now - issued <= 2000:
        return
    try:
        result = plan_selected_audio(resolve, direction, scope=scope)
    except Exception as error:
        result = {'success': False, 'reason': 'audio-planning-error', 'error_type': type(error).__name__}
    result.update(protocol='native-audio-plan-v3', token=job['token'], issued_ms=int(time.time() * 1000))
    directory = base / 'audio-channel-plans'
    directory.mkdir(parents=True, exist_ok=True)
    temporary = directory / (job['token'] + '.tmp')
    temporary.write_text(json.dumps(result))
    os.replace(temporary, directory / (job['token'] + '.json'))
