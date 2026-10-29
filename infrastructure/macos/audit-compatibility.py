#!/usr/bin/env python3
"""Inspect built artifacts; never infer live OS support from a deployment target."""
import json, pathlib, plistlib, re, subprocess
root=pathlib.Path(__file__).resolve().parents[2]
manifest=json.loads((root/'build/macos/qt-generated/build-manifest.json').read_text())
target=manifest['macos_deployment_target']
expected={'arm64','x86_64'} if manifest['macos_architectures']=='universal' else {manifest['macos_architectures']}
files=['build/macos/io-helper/Unbound.dvcp.bundle/Contents/MacOS/Unbound.dvcp',
       'build/macos/io-helper/Unbound.dvcp.bundle/Contents/MacOS/UnboundWidgetProbe.dylib',
       'build/macos/OpenGate.app/Contents/MacOS/OpenGate']
rows=[]
for file in files:
    path=root/file
    arches=set(subprocess.check_output(['xcrun','lipo','-archs',str(path)],text=True).split())
    builds=subprocess.check_output(['xcrun','vtool','-show-build',str(path)],text=True)
    minimums=re.findall(r'\bminos\s+(\d+\.\d+(?:\.\d+)?)',builds)
    assert arches==expected,(file,arches,expected)
    assert len(minimums)==len(arches) and all(v==target for v in minimums),(file,minimums,target)
    subprocess.run(['/usr/bin/codesign','--verify','--strict',str(path)],check=True)
    rows.append({'file':file,'architectures':sorted(arches),'minimum_macos':minimums})
info=plistlib.loads((root/'build/macos/OpenGate.app/Contents/Info.plist').read_bytes())
assert info['LSMinimumSystemVersion']==target
print(json.dumps({'build_audit':'PASS','artifacts':rows,'resolve_build_host':manifest.get('resolve_build_host',{}),'qt_runtime_contract':manifest.get('qt_runtime_contract','unspecified'),'meaning':'Helper deployment target does not lower the Resolve application OS requirement.','live_older_macos':'not tested','live_intel':'not tested','windows_feature_parity':'not established'},indent=2))
