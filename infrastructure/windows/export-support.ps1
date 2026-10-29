# OpenGate support export. Read only: never stop Resolve or change settings.
param(
    [string]$InstallDirectory = $PSScriptRoot,
    [string]$LogDirectory = $(if ($env:UNBOUND_DIAGNOSTICS_DIR) { $env:UNBOUND_DIAGNOSTICS_DIR } else { Join-Path $env:LOCALAPPDATA 'OpenGate\Logs' }),
    [string]$OutputDirectory = [Environment]::GetFolderPath('Desktop'),
    [string]$StartupPlugin = (Join-Path $env:PROGRAMDATA 'Blackmagic Design\DaVinci Resolve\Support\IOPlugins\OpenGate.dvcp.bundle\Contents\Win64\OpenGate.dvcp'),
    [string]$WorkflowDirectory = (Join-Path $env:PROGRAMDATA 'Blackmagic Design\DaVinci Resolve\Support\Workflow Integration Plugins')
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$stamp = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssfffZ')
$name = 'OpenGate-Windows-Support-' + $stamp + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8)
$stage = Join-Path $OutputDirectory $name
$null = New-Item -ItemType Directory -Path $stage
$copied = [Collections.Generic.List[object]]::new()
$failures = [Collections.Generic.List[object]]::new()
function Copy-Evidence([string]$Source, [string]$Relative) {
    try {
        if (!(Test-Path -LiteralPath $Source -PathType Leaf)) { return }
        $destination = Join-Path $stage $Relative
        $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $destination)
        # Copy only the current bounded snapshot; source files remain untouched.
        Copy-Item -LiteralPath $Source -Destination $destination
        $copied.Add(@{ file=$Relative; bytes=(Get-Item -LiteralPath $destination).Length; sha256=(Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash })
    } catch { $failures.Add(@{ file=$Relative; error=$_.Exception.Message }) }
}
foreach ($file in @('events.jsonl','events.jsonl.1','events.jsonl.2','events.jsonl.3','events.jsonl.4','events.jsonl.5','launcher.jsonl','launcher.jsonl.1','active-session.json')) {
    Copy-Evidence (Join-Path $LogDirectory $file) ('Logs\'+$file)
}
$incidents = Join-Path $LogDirectory 'Incidents'
if (Test-Path -LiteralPath $incidents) {
    Get-ChildItem -LiteralPath $incidents -Filter '*.json' -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 20 | ForEach-Object {
        Copy-Evidence $_.FullName ('Logs\Incidents\'+$_.Name)
    }
}
foreach ($file in @('release-manifest.json','runtime-build-manifest.json','workflow-commands-manifest.json','README.txt')) { Copy-Evidence (Join-Path $InstallDirectory $file) $file }
$workflow = @()
$workflowManifest = Join-Path $InstallDirectory 'workflow-commands-manifest.json'
if(Test-Path -LiteralPath $workflowManifest -PathType Leaf) {
    try {
        $entries = @(Get-Content -LiteralPath $workflowManifest -Raw | ConvertFrom-Json)
        $names = @('OpenGate.py') + @($entries | ForEach-Object { $_.file })
        foreach($file in $names) {
            if([IO.Path]::GetFileName($file) -ne $file -or $file -notmatch '^OpenGate(?: - .+)?\.py$'){continue}
            $path = Join-Path $WorkflowDirectory $file
            $present = Test-Path -LiteralPath $path -PathType Leaf
            $hash = if($present){(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash}else{$null}
            $workflow += @{ file=$file; present=$present; sha256=$hash }
        }
    } catch { $failures.Add(@{file='workflow-commands-manifest.json';error=$_.Exception.Message}) }
}
foreach ($inventory in @(@{ directory='PageInventory'; pattern='*-WStackPage_*-*.json' }, @{ directory='WindowInventory'; pattern='*-UiMainWindow-*.json' })) {
    $source = Join-Path $LogDirectory $inventory.directory
    if (Test-Path -LiteralPath $source) {
        Get-ChildItem -LiteralPath $source -Filter $inventory.pattern -File | Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 32 | ForEach-Object {
            Copy-Evidence $_.FullName ('Logs\'+$inventory.directory+'\'+$_.Name)
        }
    }
}
$binaries = @()
foreach ($file in @('Unbound.exe','Unbound.dll')) {
    $path = Join-Path $InstallDirectory $file
    if (Test-Path -LiteralPath $path) {
        $item = Get-Item -LiteralPath $path
        $binaries += @{ file=$file; version=$item.VersionInfo.FileVersion; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash }
    }
}
$resolve = @(Get-Process -Name Resolve -ErrorAction SilentlyContinue | ForEach-Object {
    $version = $null
    try { $version = (Get-Item -LiteralPath $_.Path).VersionInfo.FileVersion } catch { }
    @{ pid=$_.Id; version=$version }
})
$startupPresent = Test-Path -LiteralPath $StartupPlugin -PathType Leaf
$startupHash = if($startupPresent){(Get-FileHash -LiteralPath $StartupPlugin -Algorithm SHA256).Hash}else{$null}
$receipt = @{
    schema=1; product='OpenGate'; platform='windows'; captured_utc=(Get-Date).ToUniversalTime().ToString('o')
    os_version=[Environment]::OSVersion.VersionString; architecture=$env:PROCESSOR_ARCHITECTURE
    powershell=$PSVersionTable.PSVersion.ToString(); resolve=$resolve; binaries=$binaries; workflow=$workflow
    startup_plugin=@{present=$startupPresent;sha256=$startupHash}; files=@($copied.ToArray()); errors=@($failures.ToArray())
    limitations='Live log snapshot; records may continue after export. Build/fixture results do not establish live acceptance. Missing logs can mean the helper did not load. Native crash attribution is unknown.'
}
$receipt | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $stage 'support-manifest.json') -Encoding UTF8
@'
OpenGate Windows diagnostic export

Send the ZIP to the support team, with the approximate time and steps that caused the problem.
Describe the page, panel and expected result; screenshots are useful for painting issues.
events.jsonl uses the same event names and envelope as Mac: OpenGate component,
module, build, session, event_sequence, UTC time, input and observed signals.
An observed click is evidence of input, not proof that a command succeeded.
No project, media, Resolve database or native preference files are copied, and
typed editor contents are excluded. Settings are left in place. Nothing is uploaded automatically.
Keep the folder if ZIP creation fails. Export again after reproducing a problem.
'@ | Set-Content -LiteralPath (Join-Path $stage 'READ-ME.txt') -Encoding UTF8
$zip = $stage + '.zip'
Compress-Archive -LiteralPath $stage -DestinationPath $zip -CompressionLevel Optimal
Write-Host "OpenGate support ZIP: $zip"
Write-Host 'Send this file with your reproduction steps. No files were uploaded.'
