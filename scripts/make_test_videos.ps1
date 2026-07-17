param(
    [string]$FfmpegExe = "ffmpeg",
    [string]$OutDir = (Join-Path $PSScriptRoot "..\tests\assets")
)

$ErrorActionPreference = "Stop"
New-Item -ItemType Directory -Force $OutDir | Out-Null

& $FfmpegExe -y -hide_banner -loglevel error -f lavfi -i "testsrc2=duration=10:size=640x360:rate=30" -pix_fmt yuv420p (Join-Path $OutDir "sample_10s.mp4")
& $FfmpegExe -y -hide_banner -loglevel error -f lavfi -i "testsrc2=duration=3:size=360x640:rate=30" -pix_fmt yuv420p (Join-Path $OutDir "portrait_9x16.mp4")

$flat = Join-Path $OutDir "_tmp_flat.mp4"
& $FfmpegExe -y -hide_banner -loglevel error -f lavfi -i "testsrc2=duration=3:size=640x360:rate=30" -pix_fmt yuv420p $flat
& $FfmpegExe -y -hide_banner -loglevel error -display_rotation 90 -i $flat -c copy (Join-Path $OutDir "rotated_90.mp4")
Remove-Item $flat

& $FfmpegExe -y -hide_banner -loglevel error -f lavfi -i "sine=frequency=440:duration=3" -c:a aac (Join-Path $OutDir "audio_only.mp4")

$bytes = New-Object byte[] 4096
(New-Object Random 12345).NextBytes($bytes)
[IO.File]::WriteAllBytes((Join-Path $OutDir "garbage.mp4"), $bytes)

Get-ChildItem $OutDir | Select-Object Name, Length
