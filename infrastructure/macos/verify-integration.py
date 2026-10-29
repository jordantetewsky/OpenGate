#!/usr/bin/env python3
"""Run the complete fixture matrix; retain failures and exact source provenance."""
import argparse
import os
import datetime
import hashlib
import json
import shutil
from pathlib import Path
import subprocess
import sys
import time

root = Path(__file__).resolve().parents[2]
def git(*args):
    return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()

parser=argparse.ArgumentParser(description=__doc__)
modes=parser.add_mutually_exclusive_group()
modes.add_argument('--functional-only',action='store_true',help='Run functional fixtures; exclude the performance benchmark without claiming its gate passed.')
modes.add_argument('--hidden-only',action='store_true',help='Run only reviewed hidden fixtures; never native window/drag tests.')
modes.add_argument('--displayed-only',action='store_true',help='Run the native-window fixture matrix; requires Resolve closed.')
args=parser.parse_args()
resolve = subprocess.run(['pgrep', '-x', 'Resolve'], capture_output=True, text=True)
if not args.hidden_only and resolve.returncode != 1:
    print('NOT RUN: Resolve must be closed and its process state verifiable.', flush=True)
    sys.exit(2)

commit = git('rev-parse', 'HEAD')
stamp = ('displayed-' if args.displayed_only else 'hidden-' if args.hidden_only else 'functional-' if args.functional_only else '') + datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%SZ')
output = root / 'build/macos/verification' / (commit[:10] + '-' + stamp)
output.mkdir(parents=True, exist_ok=False)
tests = ['crash-session','workspace-headers','first-install-profile','settings-profile','modular-layout-recovery','attached-toolbar-reveal','native-keyboard-navigation','workflow-command-menu','panel-command-menu','source-presentation','edit-page-lifecycle','workflow-command','workflow-wrappers','panel-scale','native-presentation','native-edit-command','workspace-restoration','panel-accessories', 'panel-commands', 'timeline-control', 'window-stack-policy', 'build-manifest', 'panel-placement', 'diagnostics', 'layout-plans',
         'workspace-pages', 'workspace-menu', 'panel-sessions', 'panel-groups',
         'native-panel-positions', 'native-arrangements', 'native-workspace-ui', 'workspace-reopen', 'group-signal-lifetimes', 'controls-hidden', 'reliability', 'instrumentation', 'responsiveness']
if args.functional_only:
    tests.remove('responsiveness')
if args.hidden_only:
    tests=['crash-session','workspace-headers','first-install-profile','settings-profile','modular-layout-recovery','attached-toolbar-reveal','native-keyboard-navigation','workflow-command-menu','panel-command-menu','source-presentation','edit-page-lifecycle','workflow-command','workflow-wrappers','panel-scale','native-presentation','native-edit-command','workspace-restoration','panel-accessories','panel-commands','timeline-control','window-stack-policy','workspace-menu','build-manifest']
tests += ['shared-diagnostics','shared-layout','shared-ownership','shared-sessions','panel-caller-flow','keyframes-panel-flow','project-preset-restore','display-placement','pinned-workspace-header','surface-page-navigation','native-tab-viewport','native-preset-bindings','interaction-audit','timeline-overflow','background-flow', 'page-workspace-state','page-arrangement-controls','page-panel-inventory','native-surface-doors','native-entry-flow','clean-arrangements','opengate-first-open','resolve-layout-policy','panel-title-gesture','vacated-contents','stack-docking','stack-docking-flow','startup-readiness']
tests += ['control-launch-requests','retained-window-close','panel-menu-preparation']
if args.displayed_only:
    tests=['panel-sessions','panel-groups','native-arrangements','native-workspace-ui','workspace-reopen','reliability','controls-hidden','viewer-expansion-latency','mixer-mounted']
receipt = {'matrix': 'displayed' if args.displayed_only else 'hidden' if args.hidden_only else 'functional' if args.functional_only else 'full', 'commit': commit, 'branch': git('branch', '--show-current'),
           'dirty_at_start': bool(git('status', '--porcelain')),
           'source_lock_sha256': hashlib.sha256((root / 'infrastructure/macos/integration-features.json').read_bytes()).hexdigest(),
           'scope': ('Hidden fixtures only; displayed/native-window and responsiveness checks excluded' if args.hidden_only else 'Fixtures only; does not establish live Resolve or GPU/playback acceptance'),
           'performance_gate': 'not run; previously failed; no smoothness claim' if args.functional_only or args.hidden_only or args.displayed_only else 'included',
           'tests': [], 'all_passed': False}
def save():
    temporary = output / 'receipt.tmp'
    temporary.write_text(json.dumps(receipt, indent=2) + '\n')
    temporary.replace(output / 'receipt.json')

save()
for name in tests:
    script = root / 'infrastructure/macos' / ('test-' + name + ('.py' if name in ('build-manifest','first-install-profile','clean-arrangements') else '.sh'))
    command = [sys.executable, str(script)] if script.suffix == '.py' else ['bash', str(script)]
    print('RUN: ' + name, flush=True)
    start = time.monotonic()
    started_ns = time.time_ns()
    log = output / (name + '.log')
    with log.open('w') as stream:
        try:
            environment=os.environ.copy()
            if args.hidden_only and name=='workspace-menu': environment['UNBOUND_TEST_HEADLESS']='1'
            result = subprocess.run(command, cwd=root, env=environment, stdout=stream, stderr=subprocess.STDOUT, timeout=180)
            code = result.returncode
        except subprocess.TimeoutExpired:
            stream.write('\nFixture exceeded its 180-second deadline.\n')
            code = 124
    status = 'PASS' if code == 0 else 'NOT RUN' if code == 2 else 'FAIL'
    entry = {'name': name, 'status': status, 'exit_code': code,
             'elapsed_seconds': round(time.monotonic()-start, 3), 'log': log.name}
    if name == 'responsiveness':
        report = root / 'build/macos/responsiveness-report.json'
        if report.is_file() and report.stat().st_mtime_ns >= started_ns:
            artifact = output / report.name
            shutil.copyfile(report, artifact)
            entry['report'] = artifact.name
            entry['report_sha256'] = hashlib.sha256(artifact.read_bytes()).hexdigest()
    receipt['tests'].append(entry)
    save()
    print(status + ': ' + name + ' (' + str(log) + ')', flush=True)

receipt['all_passed'] = all(item['status'] == 'PASS' for item in receipt['tests'])
receipt['source_unchanged'] = git('rev-parse', 'HEAD') == commit and not git('status', '--porcelain')
save()
print('Receipt: ' + str(output / 'receipt.json'), flush=True)
sys.exit(0 if receipt['all_passed'] and receipt['source_unchanged'] and not receipt['dirty_at_start'] else 1)
