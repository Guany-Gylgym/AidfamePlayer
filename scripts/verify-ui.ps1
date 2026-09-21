$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$qtRoot = Join-Path $env:USERPROFILE '.cache\AidfamePlayer\Qt\6.8.3\msvc2022_64'
$env:PATH = "$qtRoot\bin;$env:PATH"
$env:QT_PLUGIN_PATH = "$qtRoot\plugins"
$env:QT_ENABLE_HIGHDPI_SCALING = '0'
$env:AIDFAME_QA_DIR = Join-Path $projectRoot 'docs\qa'
foreach ($scale in @('1', '1.5', '2')) {
    $env:QT_SCALE_FACTOR = $scale
    $report = Join-Path $env:AIDFAME_QA_DIR "stage1-tests-$scale.txt"
    & "$projectRoot\build\vs\Release\ui_tests.exe" '-o' "$report,txt" | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "UI tests failed at scale $scale (exit $LASTEXITCODE)" }
}
