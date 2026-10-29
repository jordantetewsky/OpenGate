# Exercise the actual startup plugin and assembled UI using Inno's real install
# lookup. Only a disposable runner may create this record; preserve any install.
param([Parameter(Mandatory=$true)][string]$BuildDirectory,[switch]$Conflict)
$ErrorActionPreference='Stop'
$key='HKLM:\Software\Microsoft\Windows\CurrentVersion\Uninstall\OpenGate-CineWaltz-Prototype_is1'
if(Test-Path -LiteralPath $key){throw 'An OpenGate installation exists; refusing to replace its record for a fixture.'}
$null=New-Item -Path $key
try {
    $null=New-ItemProperty -LiteralPath $key -Name InstallLocation -PropertyType String -Value $BuildDirectory
    $env:OPENGATE_TEST_IO_BOOTSTRAP='1'
    $ErrorActionPreference='Continue'
    if($Conflict) {
        & (Join-Path $BuildDirectory 'OpenGateIOConflictTests.exe') (Join-Path $BuildDirectory 'Unbound.dll') (Join-Path $BuildDirectory 'OpenGate.dvcp')
    } else {
        & (Join-Path $BuildDirectory 'OpenGateProductionFlowTests.exe') (Join-Path $BuildDirectory 'Unbound.dll')
    }
    $result=$LASTEXITCODE
    $ErrorActionPreference='Stop'
} finally {
    Remove-Item Env:OPENGATE_TEST_IO_BOOTSTRAP -ErrorAction SilentlyContinue
    Remove-Item -LiteralPath $key
}
exit $result
