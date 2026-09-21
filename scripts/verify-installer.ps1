$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if (Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\AidfamePlayerTestHarness') {
    throw 'An existing preview registration must not be changed by this test.'
}
$target = Join-Path $projectRoot ('build\installer-smoke-' + [Guid]::NewGuid().ToString('N'))
New-Item $target -ItemType Directory | Out-Null
$installer = Join-Path $projectRoot 'release\AidfamePlayer_TestHarness.exe'
$process = Start-Process $installer -ArgumentList "/S /D=$target" -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw "Installer failed: $($process.ExitCode)" }
if (!(Test-Path "$target\vcruntime140.dll")) { throw 'Missing app-local runtime.' }
if (!(Test-Path "$target\platforms\qwindows.dll")) { throw 'Missing Qt platform plugin.' }
$payload = Join-Path $projectRoot 'out\Release'
foreach ($file in (Get-ChildItem $payload -File -Recurse | Where-Object Name -ne 'Preview-Notice.txt')) {
    $relative = $file.FullName.Substring($payload.Length + 1)
    if ((Get-FileHash $file.FullName).Hash -ne (Get-FileHash (Join-Path $target $relative)).Hash) {
        throw "Installed payload mismatch: $relative"
    }
}
$sentinel = Join-Path $target 'unrelated-user-file.txt'
'Preserve this unrelated file.' | Set-Content $sentinel
$savedPath = $env:PATH
$savedPlugins = $env:QT_PLUGIN_PATH
try {
    $env:PATH = "$env:SystemRoot\System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = $null
    $player = Start-Process "$target\AidfamePlayer.exe" -PassThru
    if (!$player.WaitForInputIdle(15000)) { throw 'Player did not become responsive.' }
    Start-Sleep -Seconds 3
    $player.Refresh()
    if ($player.HasExited -or $player.MainWindowHandle -eq 0) { throw 'Player window unavailable.' }
    $blocked = Start-Process $installer -ArgumentList "/S /D=$target" -WindowStyle Hidden -PassThru -Wait
    if ($blocked.ExitCode -eq 0) { throw 'Installer should refuse replacing a running executable.' }
    $blocked = Start-Process "$target\Uninstall.exe" -ArgumentList "/S _?=$target" -WindowStyle Hidden -PassThru -Wait
    if ($blocked.ExitCode -eq 0 -or !(Test-Path "$target\AidfamePlayer.exe")) { throw 'Uninstaller should preserve a running installation.' }
    if (!$player.CloseMainWindow()) { throw 'Player could not close gracefully.' }
    if (!$player.WaitForExit(10000) -or $player.ExitCode -ne 0) { throw 'Player failed on close.' }
} finally {
    $env:PATH = $savedPath
    $env:QT_PLUGIN_PATH = $savedPlugins
}
$process = Start-Process "$target\Uninstall.exe" -ArgumentList "/S _?=$target" -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw 'Uninstaller failed.' }
if (Test-Path "$target\AidfamePlayer.exe") { throw 'Player was not uninstalled.' }
if (!(Test-Path $sentinel)) { throw 'Uninstaller removed unrelated data.' }
if (Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\AidfamePlayerTestHarness') { throw 'Uninstall registration was not removed.' }
Write-Output 'PASS: installed payload hashes, offline runtime startup, graceful exit, uninstall, unrelated file preservation.'
Write-Output "Evidence directory retained: $target"
