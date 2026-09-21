param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [switch]$SkipTests,
    [switch]$Deploy,
    [string]$QtVersion = '6.8.3',
    [string]$BuildDirectory = 'build\vs'
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer is required.' }
$vsRoot = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'MSVC x64 tools are required.' }
$cmake = Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
$qtRoot = Join-Path $env:USERPROFILE ".cache\AidfamePlayer\Qt\$QtVersion\msvc2022_64"
if (!(Test-Path -LiteralPath "$qtRoot\bin\Qt6Core.dll")) { throw 'Install the pinned Qt SDK using scripts/setup-qt.ps1.' }
$buildRoot = Join-Path $projectRoot $BuildDirectory
$generator = if ((Split-Path $vsRoot -Parent) -match '18') { 'Visual Studio 18 2026' } else { 'Visual Studio 17 2022' }
& $cmake -S $projectRoot -B $buildRoot -G $generator -A x64 "-DCMAKE_PREFIX_PATH=$qtRoot"
if ($LASTEXITCODE -ne 0) { throw 'CMake configure failed.' }
& $cmake --build $buildRoot --config $Configuration --parallel
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
$savedPath = $env:PATH
$savedPluginPath = $env:QT_PLUGIN_PATH
try {
    $env:PATH = "$qtRoot\bin;$savedPath"
    $env:QT_PLUGIN_PATH = "$qtRoot\plugins"
    if (!$SkipTests) {
        & $ctest --test-dir $buildRoot -C $Configuration --output-on-failure
        if ($LASTEXITCODE -ne 0) { throw 'Tests failed.' }
    }
    if ($Deploy) {
        $deployRoot = Join-Path $projectRoot "out\$Configuration"
        & $cmake --install $buildRoot --config $Configuration --prefix $deployRoot
        if ($LASTEXITCODE -ne 0) { throw 'Install failed.' }
        $mode = if ($Configuration -eq 'Debug') { '--debug' } else { '--release' }
        & "$qtRoot\bin\windeployqt.exe" $mode --no-translations --no-opengl-sw --no-system-d3d-compiler "$deployRoot\AidfamePlayer.exe"
        if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed.' }
    }
} finally {
    $env:PATH = $savedPath
    $env:QT_PLUGIN_PATH = $savedPluginPath
}
