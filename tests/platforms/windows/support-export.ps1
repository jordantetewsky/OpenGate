$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$root=Join-Path ([IO.Path]::GetTempPath()) ('opengate-export-test-'+[Guid]::NewGuid().ToString('N'))
$logs=Join-Path $root 'logs';$install=Join-Path $root 'install';$output=Join-Path $root 'output'
$null=New-Item -ItemType Directory -Path $logs,$install,$output
'{"component":"OpenGate","event":"input.click"}' | Set-Content (Join-Path $logs 'events.jsonl')
'{"clean_shutdown":true}' | Set-Content (Join-Path $logs 'active-session.json')
'PRIVATE_UNRELATED_CONTENT' | Set-Content (Join-Path $logs 'do-not-export.txt')
'{"SourceCommit":"fixture"}' | Set-Content (Join-Path $install 'release-manifest.json')
$before=(Get-FileHash (Join-Path $logs 'events.jsonl')).Hash
& (Join-Path $repo 'infrastructure/windows/export-support.ps1') -LogDirectory $logs -InstallDirectory $install -OutputDirectory $output
$zip=@(Get-ChildItem $output -Filter '*.zip')
if($zip.Count -ne 1){throw 'Expected one support ZIP'}
$expanded=Join-Path $root 'expanded';Expand-Archive -LiteralPath $zip[0].FullName -DestinationPath $expanded
$manifest=Get-ChildItem $expanded -Filter 'support-manifest.json' -Recurse
$data=Get-Content -LiteralPath $manifest.FullName -Raw | ConvertFrom-Json
if($data.product -ne 'OpenGate' -or $data.errors.Count -ne 0 -or $data.files.Count -ne 3){throw 'Incomplete export receipt'}
if((Get-FileHash (Join-Path $logs 'events.jsonl')).Hash -ne $before){throw 'Export altered live logs'}
if(Get-ChildItem $expanded -Filter 'do-not-export.txt' -Recurse){throw 'Unrecognized content exported'}
foreach($file in $data.files){
    $path=Join-Path $manifest.DirectoryName $file.file
    if((Get-FileHash -LiteralPath $path).Hash -ne $file.sha256){throw 'Evidence checksum mismatch'}
}
$inventories=Join-Path $logs 'WindowInventory'
$null=New-Item -ItemType Directory -Path $inventories
'{"read_only":true,"nodes":[{"name":"UiProjectsMainBarImp"}]}' | Set-Content (Join-Path $inventories 'fixture-UiMainWindow-1.json')
'PRIVATE_UNRELATED_CONTENT' | Set-Content (Join-Path $inventories 'do-not-export.json')
$plugin=Join-Path $root 'OpenGate.dvcp';'fixture plugin' | Set-Content $plugin
$workflow=Join-Path $root 'workflow';$null=New-Item -ItemType Directory -Path $workflow
'# OpenGate fixture' | Set-Content (Join-Path $workflow 'OpenGate.py')
'[{"file":"OpenGate - Show-Hide Inspector.py"},{"file":"..\\private.py"}]' | Set-Content (Join-Path $install 'workflow-commands-manifest.json')
& (Join-Path $repo 'infrastructure/windows/export-support.ps1') -LogDirectory $logs -InstallDirectory $install -OutputDirectory $output -WorkflowDirectory $workflow -StartupPlugin $plugin
$latest=Get-ChildItem $output -Directory | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
if(!(Test-Path (Join-Path $latest.FullName 'Logs\WindowInventory\fixture-UiMainWindow-1.json'))){throw 'Native structure evidence missing'}
if(Test-Path (Join-Path $latest.FullName 'Logs\WindowInventory\do-not-export.json')){throw 'Unrecognized inventory exported'}
$data=Get-Content -LiteralPath (Join-Path $latest.FullName 'support-manifest.json') -Raw | ConvertFrom-Json
if(!$data.startup_plugin.present -or $data.startup_plugin.sha256 -ne (Get-FileHash $plugin).Hash){throw 'Startup plugin checksum missing'}
if($data.workflow.Count -ne 2 -or !$data.workflow[0].present -or $data.workflow[1].present){throw 'Workflow availability not recorded accurately'}
if($data.workflow[0].sha256 -ne (Get-FileHash (Join-Path $workflow 'OpenGate.py')).Hash){throw 'Installed workflow checksum mismatch'}
$empty=Join-Path $root 'empty';$null=New-Item -ItemType Directory -Path $empty
& (Join-Path $repo 'infrastructure/windows/export-support.ps1') -LogDirectory $empty -InstallDirectory $empty -OutputDirectory $output
if(@(Get-ChildItem $output -Filter '*.zip').Count -ne 3){throw 'Missing-log export failed'}
Write-Host "PASS: exported logs/provenance, checksums, no mutation, allowlist and missing-helper case. Evidence: $root"
