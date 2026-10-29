$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..\..\..')).Path
. (Join-Path $repoRoot 'infrastructure\windows\version.ps1')
foreach ($value in @('0.0.0','0.1.0','1.2.3-rc.1','1.2.3+20261015.007','1.2.3-beta.1+build.007','65535.65535.65535')) {
    $parsed = Get-UnboundReleaseVersion -repoRoot $repoRoot -Version $value
    if ($parsed.Version -ne $value) { throw "Version was changed: $value" }
}
foreach ($value in @('v0.1.0','01.2.3','1.02.3','1.2.03','1.2','1.2.3.4','1.2.3-01','1.2.3-rc..1','1.2.3+','65536.0.0','99999999999999999999.0.0','../1.2.3')) {
    $rejected = $false
    try { $null = Get-UnboundReleaseVersion -repoRoot $repoRoot -Version $value } catch { $rejected = $true }
    if (!$rejected) { throw "Invalid/unsupported version accepted: $value" }
}
$mapped = Get-UnboundReleaseVersion -repoRoot $repoRoot -Version '12.34.56-rc.1+build.007'
if ($mapped.Major -ne 12 -or $mapped.Minor -ne 34 -or $mapped.Patch -ne 56) { throw 'Windows numeric version mapping failed' }
Write-Host 'PASS: semantic versions, prerelease/metadata, Windows numeric mapping, invalid identifiers and resource limits'
