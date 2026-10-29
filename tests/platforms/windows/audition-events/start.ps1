$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$resolveProcesses=@(Get-Process Resolve -ErrorAction SilentlyContinue)
if ($resolveProcesses.Count -ne 1) {throw 'Keep exactly one Resolve instance open on the Fairlight page.'}
$modules=@($resolveProcesses[0].Modules | ForEach-Object {$_.ModuleName})
# Keep this comparison isolated. Stop only already-loaded older interventions;
# do not inject an old diagnostic just to send it a Stop command.
foreach ($prior in @('scope','context')) {
    if ($modules -contains "Audition$($prior.Substring(0,1).ToUpper()+$prior.Substring(1)).dll") {
        $launcher=Join-Path $root "build\windows\audition-$prior\Release\audition-$prior.exe"
        if (!(Test-Path -LiteralPath $launcher)) {throw "Loaded audition-$prior helper cannot be stopped: launcher missing."}
        & $launcher --stop
        if ($LASTEXITCODE -ne 0) {throw "Could not stop the earlier audition-$prior test."}
    }
}
$launcher=Join-Path $root 'build\windows\audition-events\Release\audition-events.exe'
& $launcher --start
if ($LASTEXITCODE -ne 0) {throw 'Could not start the event-only audition test.'}
Write-Host 'Captures are automatic. Return to Resolve and try docked, floating and docked-again audition playback.'
