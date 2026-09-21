param([Parameter(Mandatory=$true)][string]$MediaDirectory)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path $PSScriptRoot -Parent
$samples = Get-Content -LiteralPath (Join-Path $projectRoot 'docs\qa\camera-media.json') -Raw | ConvertFrom-Json
$manifest = @()
foreach ($sample in $samples) {
    if ([IO.Path]::GetFileName($sample.file) -ne $sample.file) { throw 'Unexpected fixture filename.' }
    $file = Get-Item -LiteralPath (Join-Path $MediaDirectory $sample.file)
    $hash = Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256
    $manifest += [pscustomobject]@{
        file=$file.Name; bytes=$file.Length; sha256=$hash.Hash
        durationSeconds=$sample.duration
        averageContainerMbps=[math]::Round($file.Length*8/$sample.duration/1000000,3)
        width=$sample.width; height=$sample.height; fps=$sample.fps
        codec=$sample.codec; decodedPixelFormat=$sample.pixelFormat
    }
}
$manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $projectRoot 'docs\qa\camera-manifest.json') -Encoding utf8
Write-Output ('Hashed '+$manifest.Count+' read-only camera fixtures.')
