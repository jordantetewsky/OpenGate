#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Exercise the real installer transaction scripts in an isolated filesystem."""
import hashlib
import json
import os
import pathlib
import shutil
import subprocess
import tempfile
root=pathlib.Path(__file__).resolve().parents[2]
source=root/'infrastructure/macos/installer'
with tempfile.TemporaryDirectory(prefix='opengate-install-') as temporary:
    base=pathlib.Path(temporary); machine=base/'machine'; machine.mkdir()
    scripts=base/'scripts';shutil.copytree(source,scripts)
    def render(version):
      for path in scripts.iterdir():
        if path.suffix in ('.command','.sh') or path.name in ('preinstall','postinstall'):
            text=(source/path.name).read_text()
            for key,value in {'VERSION':version,'BUILD':'fixture','MIN_MAJOR':'15','MIN_MACOS':'15.0','RESOLVE_VERSION':'21.1.1'}.items():text=text.replace('@'+key+'@',value)
            path.write_text(text)
    render('0.1.0')
    app='/Applications/OpenGate.app';current='/Library/Application Support/CineWaltz/OpenGate/Current'
    inventory=[app,current];(scripts/'paths.txt').write_text('\n'.join(inventory)+'\n')
    binary=machine/(app.lstrip('/')+'/Contents/MacOS/OpenGate');binary.parent.mkdir(parents=True);binary.write_bytes(b'old executable')
    settings=machine/'Users/fixture/Library/Application Support/Unbound/Arrangements.json';settings.parent.mkdir(parents=True);settings.write_text('{"private":"user preset"}')
    before=settings.read_bytes()
    old=machine/current.lstrip('/');old.mkdir(parents=True);(old/'release-manifest.json').write_text('{"version":"0.0.1","settings_schema_written":1}')
    obsolete='/Library/Application Support/Blackmagic Design/DaVinci Resolve/Workflow Integration Plugins/OpenGate - Obsolete.py'
    retired=machine/obsolete.lstrip('/');retired.parent.mkdir(parents=True);retired.write_bytes(b'old owned command')
    unrelated=retired.parent/'User Tool.py';unrelated.write_bytes(b'user owned command')
    (old/'paths.txt').write_text('\n'.join(inventory+[obsolete])+'\n')
    legacy=machine/'Library/Application Support/Blackmagic Design/DaVinci Resolve/IOPlugins/Unbound-retired-20261021T004855Z.backup'
    (legacy/'Contents/Resources').mkdir(parents=True);(legacy/'Contents/MacOS').mkdir()
    (legacy/'Contents/Resources/build-manifest.json').write_text('{"product":"OpenGate"}')
    for name in ['Unbound.dvcp','UnboundWidgetProbe.dylib']:(legacy/'Contents/MacOS'/name).write_bytes(b'old loader')
    env=dict(os.environ,OPENGATE_TEST_ROOT=str(machine))
    product=machine/'Library/Application Support/CineWaltz/OpenGate'
    def run(name,ok=True):
        result=subprocess.run(['/bin/bash',str(scripts/name)],env=env,text=True,capture_output=True)
        assert (result.returncode==0)==ok,(name,result.stdout,result.stderr)
        assert settings.read_bytes()==before
        return result
    def prepare():
        run('preinstall')
        return pathlib.Path((product/'install.lock/transaction').read_text().strip())
    def payload(data=b'new executable',version='0.1.0'):
        binary.write_bytes(data)
        (old/'release-manifest.json').write_text(json.dumps({'version':version,'settings_schema_written':1}))
        (old/'paths.txt').write_text('\n'.join(inventory)+'\n')
        (scripts/'hashes.tsv').write_text(hashlib.sha256(data).hexdigest()+'\t'+app+'/Contents/MacOS/OpenGate\n')
    # Upgrade, same-version reinstall and downgrade use exactly the same transaction.
    for version,relation in [('0.1.0','upgrade'),('0.1.0','reinstall'),('0.0.1','downgrade')]:
        render(version)
        transaction=prepare();payload(version.encode(),version);run('postinstall')
        assert 'action='+relation in (transaction/'events.log').read_text()
        assert binary.read_bytes()==version.encode() and not (product/'install.lock').exists()
        assert not retired.exists() and not legacy.exists() and unrelated.read_bytes()==b'user owned command'
    # Failed verification restores the old binary and old metadata.
    expected=binary.read_bytes();expected_meta=(old/'release-manifest.json').read_bytes()
    transaction=prepare();payload();binary.write_bytes(b'corrupt')
    run('postinstall',False)
    assert binary.read_bytes()==expected and (old/'release-manifest.json').read_bytes()==expected_meta
    assert not (product/'install.lock').exists()
    # Interrupted replacement leaves a double-click recovery action, not a lost backup.
    transaction=prepare();payload(b'interrupted')
    result=subprocess.run(['/bin/bash',str(transaction/'Restore Previous OpenGate.command')],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    assert binary.read_bytes()==expected and not (product/'install.lock').exists()
    # A newer data writer blocks before replacement.
    (old/'release-manifest.json').write_text('{"settings_schema_written":2}')
    run('preinstall',False);assert binary.read_bytes()==expected and not (product/'install.lock').exists()
    (old/'release-manifest.json').write_bytes(expected_meta)
    # Uninstall checkpoints all managed files and leaves the user preset untouched.
    shutil.copy(scripts/'paths.txt',old/'paths.txt')
    shutil.copy(scripts/'transaction.sh',old/'transaction.sh')
    shutil.copy(scripts/'rollback.command',old/'rollback.command')
    shutil.copy(scripts/'uninstall.command',old/'uninstall.command')
    result=subprocess.run(['/bin/bash',str(old/'uninstall.command')],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    assert not binary.exists() and not old.exists() and settings.read_bytes()==before
    transactions=sorted((product/'Transactions').iterdir(),key=lambda p:p.stat().st_mtime_ns)
    recovery=transactions[-1]/'Restore Previous OpenGate.command'
    result=subprocess.run(['/bin/bash',str(recovery)],env=env,capture_output=True,text=True)
    assert result.returncode==0,result.stderr
    assert binary.read_bytes()==expected and settings.read_bytes()==before
    print('PASS: upgrade/reinstall/downgrade, corrupt payload rollback, interrupted recovery, newer-schema rejection, uninstall/restore; user presets unchanged')
