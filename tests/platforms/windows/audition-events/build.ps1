param([string]$QtPath = "$env:USERPROFILE\Qt\5.15.2\msvc2019_64")
$ErrorActionPreference='Stop'
Set-StrictMode -Version Latest
$root=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
$build=Join-Path $root 'build\windows\audition-events'
function Invoke-Checked([string]$Program,[string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {throw "$Program failed with exit code $LASTEXITCODE"}
}
Invoke-Checked 'cmake' @('-S',$PSScriptRoot,'-B',$build,'-G','Visual Studio 17 2022','-A','x64',"-DCMAKE_PREFIX_PATH=$QtPath")
Invoke-Checked 'cmake' @('--build',$build,'--config','Release','--','/m:1','/nr:false')
. (Join-Path $root 'infrastructure\windows\audit-dependencies.ps1')
$dependencies=Get-UnboundDependencies -exe (Join-Path $build 'Release\audition-events.exe') -dll (Join-Path $build 'Release\AuditionEvents.dll')
$savedPath=$env:PATH; $savedPlatform=$env:QT_QPA_PLATFORM
try {
    $env:PATH="$QtPath\bin;$env:PATH"; $env:QT_QPA_PLATFORM='offscreen'
    Invoke-Checked (Join-Path $build 'Release\audition-events-tests.exe') @()
} finally {$env:PATH=$savedPath; $env:QT_QPA_PLATFORM=$savedPlatform}
$dependencies | ConvertTo-Json | Set-Content (Join-Path $build 'dependencies.json')
Write-Host "Activation test ready: $build\Release\audition-events.exe"
