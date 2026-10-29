param([string]$QtPath = "$env:USERPROFILE\Qt\5.15.2\msvc2019_64")
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if (!(Test-Path -LiteralPath (Join-Path $QtPath 'lib\cmake\Qt5\Qt5Config.cmake'))) {
    throw "Qt 5.15.2 development files not found at $QtPath"
}
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$build = Join-Path $root 'build\windows\diagnostic'
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
Invoke-Checked 'cmake' @('-S', (Join-Path $root 'tests\platforms\windows\diagnostic'), '-B', $build, '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_PREFIX_PATH=$QtPath")
Invoke-Checked 'cmake' @('--build', $build, '--config', 'Release', '--', '/m:1', '/nr:false')
$savedPath = $env:PATH
$savedPlatform = $env:QT_QPA_PLATFORM
try {
    $env:PATH = "$QtPath\bin;$env:PATH"
    $env:QT_QPA_PLATFORM = 'offscreen'
    foreach ($mode in @('load-only','bootstrap-only','no-placement','no-snap','no-decorations','no-polling')) {
        Invoke-Checked (Join-Path $build 'Release\diagnostic-smoke.exe') @('--diagnostic-test', "--diagnostic=$mode", (Join-Path $build 'Release\Unbound.dll'))
    }
} finally {
    $env:PATH = $savedPath
    $env:QT_QPA_PLATFORM = $savedPlatform
}
Write-Host "Diagnostic binaries ready: $build\Release (not a normal release)"
