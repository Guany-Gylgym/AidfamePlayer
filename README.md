# Aidfame Player

A Chinese-first, offline C++17 / Qt 6 Widgets / libmpv Windows player.
No account, telemetry or runtime update service.

## Source release status

This is the open-source codebase for version 1.0.0, not a universally certified
binary release. No public installer is provided until the bundled dependency
source-compliance review is complete. Existing internal packages are not public
release artifacts. See [dependency provenance](docs/Dependencies.md).

## Features and limits

Local playback, hardware decoding policies, CPU fallback, 0.5x-5x speeds,
seek/volume shortcuts, aspect controls, downscaling caps, media information,
mini/fullscreen, PNG/JPEG screenshots, MP4 recording, JSON settings/history,
scoped cache cleanup and Windows file handlers are implemented.

Recording re-encodes the watched source interval after Stop; it is not desktop
capture. Resolution caps do not guarantee reduced source decoding cost.
Intel QSV is disabled. BRAW has a plugin contract, not a bundled decoder.
Windows 10/Intel hardware are unvalidated; 8K is not certified zero-drop.
See the [test scope](docs/Public-Testing.md).

## Build

Install Git, Python, Visual Studio with Desktop development with C++, the Windows
SDK and CMake tools. NSIS is needed only for local packaging.
Run `scripts/setup-qt.ps1`, `scripts/setup-mpv.ps1`, `scripts/generate-fixtures.ps1`, and `scripts/generate-format-fixtures.ps1`, then `scripts/build.ps1 -Configuration Release -Deploy`. Use `-Configuration Debug` for debug checks. Qt is stored in the user's dedicated ASCII cache directory. CMake remains authoritative; the generated Visual Studio 2026 solution is `build/vs/AidfamePlayer.slnx`.

Output is `out/Release/AidfamePlayer.exe`. Development setup downloads dependencies;
the installed player operates offline. See the developer guide for toolchain and
test instructions. Local packaging remains unsigned with an internal-use notice.

## Documentation

- [Internal contracts](docs/API-Contract.md)
- [BRAW plugin boundary](docs/BRAW-Plugin.md)
- [Developer guide](docs/Developer-Guide.md)
- [Public test scope](docs/Public-Testing.md)
- [Contributing](CONTRIBUTING.md)
- [Security reporting](SECURITY.md)

## License

Project-owned source, scripts and documentation are **GPL-3.0-or-later**.
See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md). Third-party components retain
their respective licenses. Never upload private footage or unredacted logs.
