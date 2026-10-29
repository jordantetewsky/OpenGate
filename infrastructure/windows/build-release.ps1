param(
    [string]$Version = '',
    [int]$BuildNumber = 0,
    [string]$QtPath = "$env:USERPROFILE\Qt\5.15.2\msvc2019_64"
)
$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-StrictMode -Version Latest
. (Join-Path $PSScriptRoot 'version.ps1')
$releaseVersion = Get-UnboundReleaseVersion -repoRoot $repoRoot -Version $Version
$version = $releaseVersion.Version
$buildId = (Get-Date).ToUniversalTime().ToString('yyyyMMddTHHmmssZ')
$sourceCommit = (& git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify the Windows source commit.' }
$sourceStatus = @(& git -C $repoRoot status --porcelain)
$sourceDirty = $sourceStatus.Count -gt 0
if ($BuildNumber -lt 0 -or $BuildNumber -gt 65535) { throw 'BuildNumber must be 0..65535 (the Windows numeric revision; default 0).' }
if (!(Test-Path -LiteralPath (Join-Path $QtPath 'lib\cmake\Qt5\Qt5Config.cmake'))) { throw "Qt 5.15.2 development files not found at $QtPath" }
$releaseRoot = Join-Path $repoRoot 'release\win'
New-Item -ItemType Directory -Force -Path $releaseRoot | Out-Null
$mutex = New-Object Threading.Mutex($false, 'Local\UnboundWindowsReleaseBuild')
$locked = $false
. (Join-Path $PSScriptRoot 'generate-icons.ps1')
. (Join-Path $PSScriptRoot 'audit-dependencies.ps1')
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    $command = Get-Command $Program -ErrorAction Stop
    $previousPreference = $ErrorActionPreference
    try {
        # Windows PowerShell wraps native stderr as NativeCommandError. Retain
        # every diagnostic and let the executable finish its assertions; its
        # exit code remains authoritative, including crashes and failures.
        $ErrorActionPreference = 'Continue'
        & $command @Arguments
        $exitCode = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($exitCode -ne 0) { throw "$Program failed with exit code $exitCode" }
}
$fixtureResults = [Collections.Generic.List[object]]::new()
function Save-FixtureReceipt([bool]$Complete) {
    $directory = Join-Path $repoRoot 'build\windows\fixtures'
    $null = New-Item -ItemType Directory -Force -Path $directory
    $failed = @($fixtureResults | Where-Object { $_.status -eq 'FAIL' })
    @{ schema=1; product='OpenGate'; source_commit=$sourceCommit; source_dirty=$sourceDirty;
       complete=$Complete; all_passed=($Complete -and $fixtureResults.Count -gt 0 -and $failed.Count -eq 0);
       scope='Compiled fixtures; no installation, live Resolve, GPU or physical keyboard acceptance'; tests=@($fixtureResults.ToArray()) } |
        ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $directory 'fixture-receipt.json') -Encoding UTF8
}
function Invoke-Fixture([string]$Program, [string[]]$Arguments) {
    $started = (Get-Date).ToUniversalTime(); $status='PASS'; $errorText=$null; $exitCode=$null
    Write-Host ('RUN fixture: '+(Split-Path -Leaf $Program)+' '+($Arguments -join ' '))
    try { Invoke-Checked $Program $Arguments; $exitCode=$LASTEXITCODE }
    catch {
        $status='FAIL';$errorText=$_.Exception.Message
        if($errorText -match 'exit code (-?\d+)'){$exitCode=[int]$Matches[1]}
        Write-Host ('FAIL fixture: '+$errorText)
    }
    $fixtureResults.Add(@{ program=(Split-Path -Leaf $Program); arguments=$Arguments; platform=$env:QT_QPA_PLATFORM;
        variants=@{ io_bootstrap=$env:OPENGATE_TEST_IO_BOOTSTRAP; outer_inspector=$env:OPENGATE_TEST_WINDOWS_OUTER_INSPECTOR; delayed_index=$env:OPENGATE_TEST_DELAYED_INDEX; stack_drop=$env:OPENGATE_TEST_STACK_DROP; displayed_mixer=$env:OPENGATE_TEST_DISPLAYED_MIXER; retained_close=$env:OPENGATE_TEST_RETAINED_CLOSE };
        status=$status; exit_code=$exitCode; error=$errorText; started_utc=$started.ToString('o');
        elapsed_seconds=((Get-Date).ToUniversalTime()-$started).TotalSeconds })
    Save-FixtureReceipt $false
}

try {
    try { $locked = $mutex.WaitOne(0) } catch [Threading.AbandonedMutexException] { $locked = $true }
    if (!$locked) { throw 'Another Unbound release build is running.' }
    $destination = Join-Path $releaseRoot $version
    if (Test-Path -LiteralPath $destination) { throw "Release already exists: $destination. Existing releases are never overwritten." }
    Build-UnboundIcons -repoRoot $repoRoot
    $build = Join-Path $repoRoot "build\windows\unbound\$version"
    Invoke-Checked 'cmake' @('-S', $repoRoot, '-B', $build,
        '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_PREFIX_PATH=$QtPath",
        "-DUNBOUND_VERSION=$version", "-DUNBOUND_BUILD=$BuildNumber", "-DUNBOUND_BUILD_ID=$buildId")
    Invoke-Checked 'cmake' @('--build', $build, '--config', 'Release', '--', '/m:1', '/nr:false')
    Invoke-Checked (Join-Path $build 'Release\UnboundLauncherTests.exe') @()
    & (Join-Path $repoRoot 'tests\platforms\windows\version\main.ps1')
    $exe = Join-Path $build 'Release\Unbound.exe'
    $dll = Join-Path $build 'Release\Unbound.dll'
    $ioPlugin = Join-Path $build 'Release\OpenGate.dvcp'
    foreach ($file in @($exe, $dll)) {
        $info = (Get-Item -LiteralPath $file).VersionInfo
        if ($info.ProductName -ne 'OpenGate' -or $info.CompanyName -ne 'CineWaltz' -or $info.ProductVersion -ne $version -or $info.FileVersion -ne $version -or
            $info.FileMajorPart -ne $releaseVersion.Major -or $info.FileMinorPart -ne $releaseVersion.Minor -or
            $info.FileBuildPart -ne $releaseVersion.Patch -or $info.FilePrivatePart -ne $BuildNumber) {
            throw "Version resource mismatch: $file"
        }
    }
    $dependencyReport = Get-UnboundDependencies -exe $exe -dll $dll -bootstrap $ioPlugin
    # Tests exercise the same shared pane code plus the standalone DLL bootstrap.
    $testBuild = Join-Path $repoRoot 'build\windows\pane-tests'
    Invoke-Checked 'cmake' @('-S', (Join-Path $repoRoot 'tests\core\pane-float'), '-B', $testBuild,
        '-G', 'Visual Studio 17 2022', '-A', 'x64', "-DCMAKE_PREFIX_PATH=$QtPath")
    Invoke-Checked 'cmake' @('--build', $testBuild, '--config', 'Release', '--', '/m:1', '/nr:false')
    $env:PATH = "$QtPath\bin;$env:PATH"
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:OPENGATE_FIXTURE_ROOT = Join-Path $repoRoot 'build\windows\fixtures'
    Invoke-Fixture 'python' @((Join-Path $repoRoot 'tests\platforms\macos\test-workflow-command-wrappers.py'), '--windows')
    Invoke-Fixture 'python' @((Join-Path $repoRoot 'tests\core\operations\test_audio_channel.py'))
    Invoke-Fixture (Join-Path $build 'Release\OpenGateWorkspaceCallerTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateDiagnosticTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateControlLaunchTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateLayoutPolicyTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateOwnershipTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateManagedSessionTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateStackDockingTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateWorkspaceRestorationTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGatePinnedHeaderTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGatePageNavigationTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateNativeEntryTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateFirstOpenTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGatePanelCallerFlowTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateKeyframesFlowTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateNativePresetTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGatePanelCommandTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGatePanelMenuPreparationTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGatePresetPolicyTests.exe') @()
    Invoke-Fixture (Join-Path $build 'Release\OpenGateProductionFlowTests.exe') @($dll)
    $env:OPENGATE_TEST_WINDOWS_OUTER_INSPECTOR='1'
    try { Invoke-Fixture (Join-Path $build 'Release\OpenGateProductionFlowTests.exe') @($dll) }
    finally { Remove-Item Env:OPENGATE_TEST_WINDOWS_OUTER_INSPECTOR }
    $env:OPENGATE_TEST_DELAYED_INDEX='1'
    try { Invoke-Fixture (Join-Path $build 'Release\OpenGateProductionFlowTests.exe') @($dll) }
    finally { Remove-Item Env:OPENGATE_TEST_DELAYED_INDEX }
    $legacyFixtureDll = Join-Path $build 'Release\OpenGateLegacyFixtureHelper.dll'
    Invoke-Fixture (Join-Path $build 'Release\OpenGateRetainedCallerTests.exe') @($legacyFixtureDll)
    $test = Join-Path $testBuild 'Release\pane-float-smoke.exe'
    Invoke-Fixture (Join-Path $testBuild 'Release\audition-activation-test.exe') @()
    Invoke-Fixture $test @()
    Invoke-Fixture $test @('--main-placement-test')
    Invoke-Fixture $test @('--settings-json-test')
    Invoke-Fixture $test @('--header-button-test')
    Invoke-Fixture $test @('--inspector-title-test')
    Invoke-Fixture $test @('--nodes-canvas-test')
    Invoke-Fixture $test @('--shutdown-test')
    Invoke-Fixture $test @('--close-all-test')
    Invoke-Fixture $test @('--standalone-test', $legacyFixtureDll)
    Invoke-Fixture $test @('--standalone-test', '--visible-start', $legacyFixtureDll)
    Invoke-Fixture $test @('--standalone-test', '--toggle-state-test', $legacyFixtureDll)
    Invoke-Fixture $test @('--standalone-test', '--color-selected-test', $legacyFixtureDll)
    Invoke-Fixture $test @('--reclaim-test', $legacyFixtureDll)
    Invoke-Fixture $test @('--fairlight-test', $legacyFixtureDll)
    Invoke-Fixture $test @('--fairlight-test', '--restore', $legacyFixtureDll)
    Invoke-Fixture $test @('--discovery-test', $legacyFixtureDll)
    $env:QT_QPA_PLATFORM = 'windows'
    try {
        Invoke-Fixture (Join-Path $build 'Release\OpenGateWindowAdapterTests.exe') @()
        Invoke-Fixture (Join-Path $build 'Release\OpenGateManagedSessionTests.exe') @('--displayed')
        $env:OPENGATE_TEST_DISPLAYED_MIXER='1'
        $env:OPENGATE_TEST_STACK_DROP='1'
        try {
            Invoke-Fixture (Join-Path $build 'Release\OpenGateProductionFlowTests.exe') @($dll)
            Invoke-Fixture 'powershell.exe' @('-NoProfile','-ExecutionPolicy','Bypass','-File',(Join-Path $repoRoot 'tests\platforms\windows\io-startup.ps1'),'-BuildDirectory',(Join-Path $build 'Release'))
            Invoke-Fixture 'powershell.exe' @('-NoProfile','-ExecutionPolicy','Bypass','-File',(Join-Path $repoRoot 'tests\platforms\windows\io-startup.ps1'),'-BuildDirectory',(Join-Path $build 'Release'),'-Conflict')
        }
        finally { Remove-Item Env:OPENGATE_TEST_DISPLAYED_MIXER; Remove-Item Env:OPENGATE_TEST_STACK_DROP }
        $env:OPENGATE_TEST_RETAINED_CLOSE='1'
        try { Invoke-Fixture (Join-Path $build 'Release\OpenGateProductionFlowTests.exe') @($dll) }
        finally { Remove-Item Env:OPENGATE_TEST_RETAINED_CLOSE }
        Invoke-Fixture $test @('--audition-taskbar-native')
        Invoke-Fixture $test @('--snap-native')
        Invoke-Fixture $test @('--captionless-native')
        Invoke-Fixture $test @('--frame-paint-native')
    }
    finally { $env:QT_QPA_PLATFORM = 'offscreen' }
    Save-FixtureReceipt $true
    $failedFixtures = @($fixtureResults | Where-Object { $_.status -eq 'FAIL' })
    if($failedFixtures.Count -gt 0){throw "$($failedFixtures.Count) Windows fixtures failed. No candidate was packaged; all results are retained."}
    # Preserve binary provenance with the candidate and installed support exporter.
    New-Item -ItemType Directory -Path $destination | Out-Null
    Copy-Item -LiteralPath $exe, $dll, $ioPlugin -Destination $destination
    Copy-Item -LiteralPath (Join-Path $repoRoot 'build\windows\workflow-commands') -Destination $destination -Recurse
    $hashes = @(Get-FileHash -LiteralPath (Join-Path $destination 'Unbound.exe'), (Join-Path $destination 'Unbound.dll'), (Join-Path $destination 'OpenGate.dvcp'))
    $workflowHashes = @(Get-ChildItem -LiteralPath (Join-Path $destination 'workflow-commands') -File | Get-FileHash -Algorithm SHA256)
    @{ Product='OpenGate'; Version=$version; ReleaseLabel="$version-prototype.$BuildNumber"; BuildId=$buildId; SourceCommit=$sourceCommit; SourceDirty=$sourceDirty; SourceStatus=$sourceStatus; Revision=$BuildNumber; BuiltAt=(Get-Date).ToString('o'); Qt=$QtPath; Files=$hashes; WorkflowFiles=$workflowHashes; Dependencies=$dependencyReport; Backend='shared-managed-workspace'; Startup='native-io-plugin'; ManagedWorkspaceParity=$false; LiveAccepted=$false } |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $build 'release-manifest.json')
    Copy-Item -LiteralPath (Join-Path $build 'release-manifest.json') -Destination $destination
    Write-Host "Unbound $version ready: $destination"
} finally {
    if ($locked) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
