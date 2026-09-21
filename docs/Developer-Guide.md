# Developer Guide

## Tools

Install Visual Studio with Desktop development with C++, an x64 Windows SDK,
Python (for the development-only dependency setup), Git, and NSIS. This host
uses Visual Studio 2026 / MSVC 14.51. CMake is taken from that Visual Studio
installation. Qt 6.8.3 MSVC 2022 x64 is pinned and dynamically linked.

From the project directory, in PowerShell:

```powershell
./scripts/setup-qt.ps1
./scripts/setup-mpv.ps1
./scripts/generate-fixtures.ps1
./scripts/generate-format-fixtures.ps1
./scripts/build.ps1 -Configuration Debug
./scripts/build.ps1 -Configuration Release -Deploy
./scripts/verify-ui.ps1
./scripts/package.ps1 -SkipBuild -TestInstall
./scripts/verify-installer.ps1
./scripts/package.ps1 -SkipBuild
```

Development setup downloads dependencies; the installed player does not.
If NSIS is not discoverable, pass `-Makensis` with its executable path.
Open `build/vs/AidfamePlayer.slnx` in Visual Studio 2026 after configuration.
CMake is the project source of truth; generated IDE files contain local paths
and should be regenerated when the repository or SDK locations change.

## Modules and lifetime

- App owns QApplication and the WinRT/MTA lifetime guard. The latter must outlive
  Qt, including its Windows platform plugin. Do not remove it as redundant COM setup.
- UI uses Qt Widgets. A native child HWND hosts mpv's D3D11 output. Mini mode
  preserves that HWND; media information uses mpv OSD instead of overlaid QWidget.
- Player owns one mpv handle, observes properties and queues wakeups to Qt.
  Keep window-affecting mpv commands asynchronous to avoid event-loop deadlock.
- Recorder owns a bounded-thread local FFmpeg subprocess and only its unique
  partial output. It is source-interval re-encoding, not desktop screen capture.
- Settings owns atomic local JSON, history and ownership-checked cache cleanup.
  Corrupt/unreadable settings that cannot be backed up must not be overwritten.
- Windows handler registration writes only application-owned HKCU entries and
  invokes Windows Settings for user-confirmed defaults. Never patch UserChoice.
- BRAW ABI is defined in src/Player/DecoderPluginApi.h. It does not imply a
  bundled proprietary decoder; consult BRAW-Plugin.md before SDK integration.

## Hardware and offline boundaries

Direct D3D11VA/DXVA2 and NVDEC copy-back are tested on this host. CPU scaling
requires copy-back surfaces. Quality caps do not reduce source decoding work.
The core's decoder list includes FFmpeg QSV codecs, but no Intel device exists
here; the shipping QSV option is disabled. A raw codec-preference probe only
proves fallback without Intel, not working Intel acceleration.

Input is restricted to local regular files, with URLs, remote drives, reparse
points and external playlist/script/config loading rejected. FFmpeg's protocol
whitelist is local-file-only. DLL searches exclude the current working directory.
Never relax these boundaries just to make a fixture open.

## Long tests

Set Qt bin on PATH and QT_PLUGIN_PATH to its plugins directory, then use:

```powershell
$env:AIDFAME_MEDIA_DIR = 'D:/path/to/camera-clips'
$env:AIDFAME_MEDIA_REPORT = 'docs/qa/real-media.json'
./build/vs/Release/media_tests.exe

$env:AIDFAME_FULL_MEDIA = '1'
$env:QTEST_FUNCTION_TIMEOUT = '7200000'
./build/vs/Release/media_tests.exe

./build/vs/Release/soak_runner.exe 'D:/path/to/clip.mp4' 7200 'docs/qa/soak.json'
```

Check the final process exit code AND the report's finished/passed fields.
Intermediate reports are not successes. Soak uses real elapsed time and detects
stalls; inspect memory trends and dropped-frame data separately. Hash fixtures
with camera-manifest.ps1 and collect whole-device NVIDIA observations with
sample-gpu.ps1. Do not report whole-GPU counters as exclusive process usage.

Build the long-test executables explicitly with
`cmake --build build/vs --config Release --target media_tests soak_runner`.
They are excluded from the default build so routine development does not try to
relink an executable that is still running. Use a snapshot copy of runner/DLL
files for long jobs if changing the core concurrently. Record the tested binary
hash; do not equate a snapshot with untested later playback changes.

## Packaging and scope

The NSIS file manifest is explicit and uninstall never recursively deletes the
installation directory. In-use application files must block install/uninstall
before any mutation. Test harness uses HKCU/no shared shortcuts; it does not
replace clean-VM administrator testing. The current package is unsigned.

Project source is offered under GPL-3.0-or-later. Public source publication does
not authorize redistribution of unaudited bundled binaries. Preserve third-party
licenses/provenance and resolve corresponding-source obligations before a public
installer release. See Dependencies.md and Public-Testing.md for limits.
