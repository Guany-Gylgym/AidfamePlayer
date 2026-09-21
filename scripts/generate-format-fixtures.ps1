$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$ffmpeg = Join-Path $projectRoot '.tools\python\Lib\site-packages\imageio_ffmpeg\binaries\ffmpeg-win-x86_64-v7.1.exe'
$mediaRoot = Join-Path $projectRoot 'Tests\FormatMedia'
New-Item -ItemType Directory -Force $mediaRoot | Out-Null
$cases = @(
    @{Name='prores.mov'; Size='1920x1080'; Codec=@('-c:v','prores_ks','-profile:v','3','-pix_fmt','yuv422p10le'); Audio='pcm_s16le'},
    @{Name='dnxhd.mxf'; Size='1920x1080'; Codec=@('-c:v','dnxhd','-b:v','120M','-pix_fmt','yuv422p'); Audio='pcm_s16le'},
    @{Name='av1.mkv'; Size='1920x1080'; Codec=@('-c:v','libaom-av1','-cpu-used','8','-crf','38','-b:v','0','-row-mt','1'); Audio='aac'},
    @{Name='vp9.webm'; Size='1920x1080'; Codec=@('-c:v','libvpx-vp9','-deadline','realtime','-cpu-used','8','-crf','38','-b:v','0'); Audio='libopus'},
    @{Name='hevc-8k.mp4'; Size='7680x4320'; Codec=@('-c:v','libx265','-preset','ultrafast','-x265-params','log-level=error:pools=4:frame-threads=2','-crf','30','-pix_fmt','yuv420p10le'); Audio='aac'},
    @{Name='legacy.avi'; Size='1280x720'; Codec=@('-c:v','mpeg4','-q:v','4'); Audio='mp3'},
    @{Name='legacy.wmv'; Size='1280x720'; Codec=@('-c:v','wmv2','-b:v','5M'); Audio='wmav2'},
    @{Name='h264.flv'; Size='1280x720'; Codec=@('-c:v','libx264','-preset','ultrafast','-crf','20'); Audio='aac'},
    @{Name='h264.ts'; Size='1920x1080'; Codec=@('-c:v','libx264','-preset','ultrafast','-crf','18'); Audio='aac'},
    @{Name='h264.mts'; Size='1920x1080'; Codec=@('-c:v','libx264','-preset','ultrafast','-crf','18','-f','mpegts'); Audio='ac3'},
    @{Name='h264.m2ts'; Size='1920x1080'; Codec=@('-c:v','libx264','-preset','ultrafast','-crf','18','-mpegts_m2ts_mode','1','-f','mpegts'); Audio='ac3'}
)
foreach ($case in $cases) {
    $output = Join-Path $mediaRoot $case.Name
    if (Test-Path -LiteralPath $output) {
        & $ffmpeg -hide_banner -loglevel error -xerror -i $output -f null NUL
        if ($LASTEXITCODE -ne 0) { throw "Existing fixture is invalid; inspect before regenerating: $output" }
        continue
    }
    Write-Output ('Generating '+$case.Name)
    $options = @('-hide_banner','-loglevel','error','-filter_threads','2','-f','lavfi','-i',('testsrc2=size='+$case.Size+':rate=25'),'-f','lavfi','-i','sine=frequency=440:sample_rate=48000','-t','8')
    $options += $case.Codec
    $options += @('-threads','4','-c:a',$case.Audio,'-shortest','-n',$output)
    & $ffmpeg @options
    if ($LASTEXITCODE -ne 0) { throw ('Fixture generation failed: '+$case.Name) }
}
