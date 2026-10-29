function Get-UnboundReleaseVersion {
    param([string]$repoRoot, [string]$Version = '')
    if (!$Version) { $Version = (Get-Content -LiteralPath (Join-Path $repoRoot 'VERSION') -Raw).Trim() }
    $number = '(?:0|[1-9][0-9]*)'
    $pre = '(?:0|[1-9][0-9]*|[0-9]*[a-zA-Z-][0-9a-zA-Z-]*)'
    $pattern = "\A(?<major>$number)\.(?<minor>$number)\.(?<patch>$number)(?:-$pre(?:\.$pre)*)?(?:\+[0-9a-zA-Z-]+(?:\.[0-9a-zA-Z-]+)*)?\z"
    $match = [regex]::Match($Version, $pattern)
    if (!$match.Success) { throw "Invalid semantic version: $Version. Use MAJOR.MINOR.PATCH with optional prerelease/build metadata, without a v prefix." }
    $parts = @()
    foreach ($name in @('major','minor','patch')) {
        $value = $match.Groups[$name].Value
        if ($value.Length -gt 5 -or [int]$value -gt 65535) { throw "Version component exceeds the Windows resource limit of 65535: $value" }
        $parts += [int]$value
    }
    [pscustomobject]@{ Version=$Version; Major=$parts[0]; Minor=$parts[1]; Patch=$parts[2] }
}
