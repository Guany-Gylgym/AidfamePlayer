param([int]$Samples = 12, [int]$IntervalSeconds = 5, [string]$Output)
$ErrorActionPreference = 'Stop'
if ($Samples -lt 2 -or $Samples -gt 720 -or $IntervalSeconds -lt 1 -or $IntervalSeconds -gt 30) { throw 'Invalid sampling bounds.' }
$projectRoot = Split-Path $PSScriptRoot -Parent
if (!$Output) { $Output = Join-Path $projectRoot 'docs\qa\gpu-samples.json' }
$rows = @()
for ($index = 0; $index -lt $Samples; $index++) {
    $gpu = & nvidia-smi --query-gpu=name,driver_version,utilization.gpu,utilization.memory,utilization.decoder,memory.used --format=csv,noheader,nounits
    if ($LASTEXITCODE -ne 0) { throw 'GPU query failed.' }
    $rows += [pscustomobject]@{utc=(Get-Date).ToUniversalTime().ToString('o'); scope='Whole NVIDIA GPU, not exclusive process attribution'; gpu=$gpu}
    $rows | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $Output -Encoding utf8
    if ($index -lt $Samples-1) { Start-Sleep -Seconds $IntervalSeconds }
}
Write-Output ('Collected '+$rows.Count+' whole-GPU samples.')
