param([string]$Makensis, [switch]$SkipBuild, [switch]$TestInstall)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
if (!$SkipBuild) { & "$PSScriptRoot\build.ps1" -Configuration Release -Deploy }
if (!$Makensis) {
    $command = Get-Command makensis -ErrorAction SilentlyContinue
    if ($command) { $Makensis = $command.Source }
    else {
        $cache = Join-Path $env:LOCALAPPDATA 'electron-builder\Cache\nsis-3.0.4.1'
        $Makensis = Get-ChildItem $cache -Filter makensis.exe -Recurse | Select-Object -First 1 -ExpandProperty FullName
    }
}
if (!$Makensis) { throw 'NSIS is required; provide -Makensis with its absolute path.' }
$payload = Join-Path $projectRoot 'out\Release'
if (!(Test-Path "$payload\AidfamePlayer.exe")) { throw 'Deploy Release first.' }
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$crt = Get-ChildItem "$vsRoot\VC\Redist\MSVC" -Directory | Where-Object Name -Match '^14\.' | Sort-Object Name -Descending | Select-Object -First 1
$crtDir = Get-ChildItem "$($crt.FullName)\x64" -Directory -Filter '*.CRT' | Select-Object -First 1
if (!$crtDir) { throw 'MSVC redistributable DLLs are required.' }
Copy-Item "$($crtDir.FullName)\*.dll" $payload
New-Item "$payload\docs" -ItemType Directory -Force | Out-Null
Copy-Item "$projectRoot\docs\Dependencies.md","$projectRoot\docs\User-Guide.md","$projectRoot\docs\User-Guide.zh-CN.md","$projectRoot\docs\Test-Report.md" "$payload\docs"
Copy-Item "$projectRoot\Installer\Internal-Notice.txt" $payload
New-Item "$payload\licenses" -ItemType Directory -Force | Out-Null
Copy-Item "$projectRoot\licenses\*.txt" "$payload\licenses"
$generated = Join-Path $projectRoot 'build\installer'
$release = Join-Path $projectRoot 'release'
New-Item $generated,$release -ItemType Directory -Force | Out-Null
$installLines = [Collections.Generic.List[string]]::new()
$removeLines = [Collections.Generic.List[string]]::new()
$files = Get-ChildItem $payload -File -Recurse | Where-Object Name -ne 'Preview-Notice.txt' | Sort-Object FullName
foreach ($file in $files) {
    $relative = $file.FullName.Substring($payload.Length + 1)
    if ($relative -match '[\r\n$\"]') { throw 'Unsafe payload filename.' }
    $directory = Split-Path $relative -Parent
    $installLines.Add('SetOutPath "$INSTDIR\' + $directory + '"')
    $installLines.Add('File "' + $file.FullName + '"')
    $removeLines.Add('Delete "$INSTDIR\' + $relative + '"')
}
Get-ChildItem $payload -Directory -Recurse | Sort-Object { $_.FullName.Length } -Descending | ForEach-Object {
    $removeLines.Add('RMDir "$INSTDIR\' + $_.FullName.Substring($payload.Length + 1) + '"')
}
$installManifest = Join-Path $generated 'install.nsh'
$removeManifest = Join-Path $generated 'uninstall.nsh'
$installLines | Set-Content $installManifest -Encoding utf8
$removeLines | Set-Content $removeManifest -Encoding utf8
$name = if ($TestInstall) { 'AidfamePlayer_TestHarness.exe' } else { 'AidfamePlayer_Setup.exe' }
$output = Join-Path $release $name
$arguments = @('/V2',"/DOUTPUT=$output","/DMANIFEST_INSTALL=$installManifest","/DMANIFEST_UNINSTALL=$removeManifest")
if ($TestInstall) { $arguments += '/DTEST_INSTALL' }
& $Makensis @arguments "$projectRoot\Installer\AidfamePlayer.nsi"
if ($LASTEXITCODE -ne 0) { throw 'NSIS compilation failed.' }
Get-FileHash $output -Algorithm SHA256
Get-Item $output | Select-Object FullName,Length
