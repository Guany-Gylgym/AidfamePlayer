$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
Push-Location $projectRoot
try {
    if (!(Test-Path '.tools\python\Scripts\python.exe')) {
        python -m venv .tools/python
        if ($LASTEXITCODE -ne 0) { throw 'Python venv creation failed.' }
    }
    & '.\.tools\python\Scripts\python.exe' -m pip install aqtinstall==3.3.0
    if ($LASTEXITCODE -ne 0) { throw 'aqtinstall installation failed.' }
    # qmake emits localized paths; use an ASCII SDK root to avoid aqt's UTF-8 decoder failure.
    $sdkRoot = Join-Path $env:USERPROFILE '.cache\AidfamePlayer\Qt'
    & '.\.tools\python\Scripts\python.exe' -m aqt install-qt windows desktop 6.8.3 win64_msvc2022_64 -O $sdkRoot --archives qtbase qtsvg qttools
    if ($LASTEXITCODE -ne 0) { throw 'Qt installation failed.' }
} finally { Pop-Location }
