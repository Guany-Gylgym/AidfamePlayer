$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$vendorRoot = Join-Path $projectRoot 'vendor\mpv'
New-Item -ItemType Directory -Force $vendorRoot | Out-Null
$archive = Join-Path $vendorRoot 'mpv-dev.7z'
$url = 'https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20260920/mpv-dev-x86_64-20260920-git-e76a35ec95.7z'
Invoke-WebRequest $url -OutFile $archive
if ((Get-FileHash $archive -Algorithm SHA256).Hash -ne '60F9102DB46AEA8CEF9BFB4345EE6A106F34FDBD1DF9587E38F0660688039341') {
    throw 'libmpv archive checksum mismatch.'
}
$extractor = Join-Path $projectRoot '.tools\7zr.exe'
if (!(Test-Path $extractor)) { Invoke-WebRequest 'https://www.7-zip.org/a/7zr.exe' -OutFile $extractor }
& $extractor x $archive "-o$vendorRoot" -y
if ($LASTEXITCODE -ne 0) { throw 'libmpv extraction failed.' }
