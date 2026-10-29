$ErrorActionPreference='Stop'
$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..\..')).Path
$tokens=$null;$parseErrors=$null
$ast=[Management.Automation.Language.Parser]::ParseFile((Join-Path $repo 'infrastructure/windows/build-release.ps1'),[ref]$tokens,[ref]$parseErrors)
if($parseErrors.Count){throw 'Build script has syntax errors'}
$function=$ast.Find({param($node) $node -is [Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq 'Invoke-Checked'},$true)
if(!$function){throw 'Production native command runner missing'}
Invoke-Expression $function.Extent.Text
$root=Join-Path ([IO.Path]::GetTempPath()) ('OpenGate command runner '+[Guid]::NewGuid().ToString('N'))
$null=New-Item -ItemType Directory -Path $root
$zero=Join-Path $root 'success.cmd';$failed=Join-Path $root 'failure.cmd'
"@echo stderr-probe 1>&2`r`n@exit /b 0" | Set-Content -LiteralPath $zero -Encoding ASCII
"@echo failing-stderr-probe 1>&2`r`n@exit /b 23" | Set-Content -LiteralPath $failed -Encoding ASCII
$zeroLog=Join-Path $root 'success.log';$failedLog=Join-Path $root 'failure.log'
Invoke-Checked $zero @() *> $zeroLog
if(!(Get-Content -LiteralPath $zeroLog -Raw).Contains('stderr-probe')){throw 'Native stderr was lost'}
$rejected=$false
try {Invoke-Checked $failed @() *> $failedLog} catch {$rejected=$_.Exception.Message.Contains('exit code 23')}
if(!$rejected){throw 'Nonzero native exit was not rejected with its original code'}
if(!(Get-Content -LiteralPath $failedLog -Raw).Contains('failing-stderr-probe')){throw 'Failed native diagnostics were lost'}
$missing=$false
try {Invoke-Checked (Join-Path $root 'does-not-exist.exe') @()} catch {$missing=$true}
if(!$missing){throw 'Missing executable was accepted'}
Write-Host "PASS: native stderr retained, zero/nonzero exits enforced, missing programs rejected, paths with spaces. Evidence: $root"
# The intentional nonzero child probe leaves LASTEXITCODE set. GitHub's
# PowerShell wrapper propagates it even though every assertion above passed.
exit 0
