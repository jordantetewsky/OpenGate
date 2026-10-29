param([string]$Version = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
. (Join-Path $PSScriptRoot 'version.ps1')
$version = (Get-UnboundReleaseVersion -repoRoot $repoRoot -Version $Version).Version
$tag = "v$version"
$commit = & git -C $repoRoot rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot read source commit.' }
$tagCommit = & git -C $repoRoot rev-parse --verify "$tag^{commit}"
if ($LASTEXITCODE -ne 0 -or $tagCommit -ne $commit) { throw "Tag $tag must point to HEAD before packaging." }
$changes = & git -C $repoRoot status --porcelain
if ($LASTEXITCODE -ne 0 -or $changes) { throw 'Commit source changes before packaging.' }
$release = Join-Path $repoRoot "release\win\$version"
$manifest = Get-Content -LiteralPath (Join-Path $repoRoot "build\windows\unbound\$version\release-manifest.json") -Raw | ConvertFrom-Json
if ($manifest.Version -ne $version) { throw 'Release manifest version mismatch.' }
foreach ($name in @('Unbound.exe', 'Unbound.dll')) {
    $file = Join-Path $release $name
    $info = (Get-Item -LiteralPath $file).VersionInfo
    if ($info.FileVersion -ne $version -or $info.ProductVersion -ne $version) { throw "Embedded version mismatch: $name" }
    $record = @($manifest.Files | Where-Object { [IO.Path]::GetFileName($_.Path) -eq $name })
    if ($record.Count -ne 1 -or (Get-FileHash -LiteralPath $file).Hash -ne $record[0].Hash) { throw "Release hash mismatch: $name" }
}
$folder = "Unbound-$version-windows-x64"
$packages = Join-Path $repoRoot 'release\packages'
New-Item -ItemType Directory -Force -Path $packages | Out-Null
$destination = Join-Path $packages "$folder.zip"
if (Test-Path -LiteralPath $destination) { throw "Package already exists: $destination" }
$readme = @"
Unbound for DaVinci Resolve $version (Windows x64)

Extract this folder, keep Unbound.exe and Unbound.dll together, and run Unbound.exe.
Unbound waits silently if Resolve is not running yet.
Use a fresh Resolve process when testing a new Unbound build.

Settings: %LOCALAPPDATA%\Unbound\Unbound.json
Source: https://github.com/wolfgangw/unbound
Source tag: $tag
Source commit: $commit
Build ID: $($manifest.BuildId)
License: MIT (see LICENSE)
"@
Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem
$stream = [IO.File]::Open($destination, [IO.FileMode]::CreateNew)
$archive = $null
$complete = $false
try {
    $archive = New-Object IO.Compression.ZipArchive($stream, [IO.Compression.ZipArchiveMode]::Create, $true)
    foreach ($name in @('Unbound.exe', 'Unbound.dll', 'LICENSE')) {
        $source = if ($name -eq 'LICENSE') { Join-Path $repoRoot $name } else { Join-Path $release $name }
        [IO.Compression.ZipFileExtensions]::CreateEntryFromFile($archive, $source, "$folder/$name", [IO.Compression.CompressionLevel]::Optimal) | Out-Null
    }
    $entry = $archive.CreateEntry("$folder/README.txt")
    $writer = New-Object IO.StreamWriter($entry.Open(), (New-Object Text.UTF8Encoding($false)))
    try { $writer.Write($readme.Replace("`r`n", "`n") + "`n") } finally { $writer.Dispose() }
    $archive.Dispose()
    $archive = $null
    $stream.Dispose()
    $complete = $true
} finally {
    if ($archive) { $archive.Dispose() }
    $stream.Dispose()
    if (!$complete) { Remove-Item -LiteralPath $destination }
}
Write-Host "Package ready: $destination"
Get-FileHash -LiteralPath $destination
