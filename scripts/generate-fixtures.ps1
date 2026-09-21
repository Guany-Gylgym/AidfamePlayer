$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
& "$projectRoot\.tools\python\Scripts\python.exe" -m pip install imageio-ffmpeg==0.6.0
if ($LASTEXITCODE -ne 0) { throw 'Test generator dependency failed.' }
$ffmpeg = (Get-ChildItem "$projectRoot\.tools\python\Lib\site-packages\imageio_ffmpeg\binaries\ffmpeg*.exe").FullName
$mediaRoot = Join-Path $projectRoot 'Tests\Media'
New-Item -ItemType Directory -Force $mediaRoot | Out-Null
if (!(Test-Path "$mediaRoot\test-h264-1080p.mp4")) {
    & $ffmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=1920x1080:rate=30' -f lavfi -i 'sine=frequency=440:sample_rate=48000' -t 12 -c:v libx264 -preset ultrafast -crf 22 -pix_fmt yuv420p -c:a aac -shortest -n "$mediaRoot\test-h264-1080p.mp4"
    if ($LASTEXITCODE -ne 0) { throw 'H.264 fixture failed.' }
}
if (!(Test-Path "$mediaRoot\test-hevc-4k.mp4")) {
    & $ffmpeg -hide_banner -loglevel error -f lavfi -i 'testsrc2=size=3840x2160:rate=30' -f lavfi -i 'sine=frequency=330:sample_rate=48000' -t 12 -c:v libx265 -preset ultrafast -x265-params 'log-level=error:pools=4' -crf 26 -pix_fmt yuv420p10le -c:a aac -shortest -n "$mediaRoot\test-hevc-4k.mp4"
    if ($LASTEXITCODE -ne 0) { throw 'HEVC fixture failed.' }
}
