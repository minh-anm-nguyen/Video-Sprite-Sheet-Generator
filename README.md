# Video Sprite Sheet Generator

A fast C++20 command line tool that turns a video into thumbnail sprite sheets plus a WebVTT map for timeline hover previews — the small preview images you see when hovering the seek bar on YouTube or Netflix.

Point it at a video and it extracts frames at even intervals, scales them into a tiled grid image, and writes one `.vtt` file that maps every time range to the matching tile.

![Sprite sheet generated from a 10 second test clip, default 5x5 grid](assets/demo_sprite.jpg)

Each cell of the sheet above is addressed by one WebVTT cue:

```vtt
WEBVTT

00:00:00.000 --> 00:00:00.400
sprite.jpg#xywh=0,0,160,90

00:00:00.400 --> 00:00:00.800
sprite.jpg#xywh=160,0,160,90
```

Any player with thumbnail track support (Video.js, Plyr, JW Player, ...) consumes these two files as-is.

## Features

- Frame-accurate selection by PTS, correct even for variable frame rate video
- Applies rotation metadata and keeps the source aspect ratio (letterbox/pillarbox)
- Splits into numbered sheets when the grid overflows; still a single WebVTT file
- JPEG or PNG output chosen by file extension; `--base-url` for CDN-hosted images
- Atomic writes (temp file + rename) and no silent overwrites without `--force`
- Progress bar with ETA on interactive terminals; silent when piped or with `--quiet`
- Ctrl+C cancels quickly and removes every file created by the run
- Flat memory profile: one sheet in RAM at a time, ~126 MB peak on 4K input

## Usage

```text
video-sprite-sheet <input> [options]

  -i, --interval <seconds>  time between thumbnails (default: duration / (cols x rows))
  -c, --cols <n>            grid columns (default: 5)
  -r, --rows <n>            grid rows (default: 5)
  -w, --thumb-width <px>    thumbnail width, even, at least 16 (default: 160)
  -o, --output <file>       sprite image path, .jpg/.jpeg/.png (default: <input>_sprite.jpg)
      --vtt <file>          WebVTT output path (default: <input>_sprite.vtt)
  -q, --quality <1-100>     JPEG quality (default: 85)
      --base-url <url>      prefix for image references inside the VTT file
      --force, --overwrite  allow overwriting existing output files
      --quiet               suppress progress output
```

```console
video-sprite-sheet movie.mp4
video-sprite-sheet movie.mp4 -i 10 -c 4 -r 6 -o out/sprite.jpg --vtt out/sprite.vtt
video-sprite-sheet movie.mp4 --base-url https://cdn.example.com/previews -q 90
```

When `--interval` is omitted the tool spreads thumbnails evenly so they fill exactly one sheet.

Exit codes: `0` success, `1` argument error, `2` input error, `3` processing error, `4` interrupted.

## Build

Windows, with Visual Studio 2022 or newer (bundled vcpkg) and CMake 3.21+:

```powershell
$env:VCPKG_ROOT = "<vcpkg path>"   # already set inside a VS Developer shell
cmake --preset windows-msvc        # first configure builds FFmpeg through vcpkg
cmake --build --preset release
build\windows-msvc\Release\video-sprite-sheet.exe --help
```

Tests (Catch2, 90+ cases):

```powershell
cmake --build --preset debug
ctest --test-dir build/windows-msvc -C Debug
```

Runtime dependencies are the FFmpeg shared libraries only: avformat, avcodec, avutil, swscale. Linux support is planned; the platform-specific code is already isolated.
