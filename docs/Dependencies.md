# Development Dependency Provenance

| Component | Pinned build | Source | Distribution status |
|---|---|---|---|
| Qt | 6.8.3 MSVC 2022 x64 | download.qt.io through aqtinstall 3.3.0 | Dynamic linking; license/source notice packaging required |
| libmpv | 20260920 git e76a35ec95 x86_64 | shinchiro/mpv-winbuild-cmake GitHub release | Prototype binary; treat as GPL unless build audit proves otherwise |
| FFmpeg recording worker and test generator | 7.1 from imageio-ffmpeg 0.6.0 | PyPI wheel | Selected runtime executable; redistribution license/source audit remains required |
| 7zr | Downloaded 26.03 | 7-zip.org | Development archive extraction only |

libmpv archive SHA-256: `60f9102db46aea8cef9bfb4345ee6a106f34fdbd1df9587e38f0660688039341` (matches release asset digest).

Project-owned source is offered under GPL-3.0-or-later. Dependencies retain their
own licenses. The FFmpeg worker's own `-L` output declares GPLv3+ and its license
is included. Selected upstream notices in licenses/ are not a complete public
binary source-compliance audit. The repository contains no runtime binaries.

Before public binary distribution, resolve the complete mpv/FFmpeg dependency
graph, exact corresponding source, patches, build options and license obligations,
plus Qt and MSVC runtime redistribution requirements. Upstream download links
alone are not a substitute for compliant corresponding-source distribution.
Local builds do not authorize redistribution of unaudited third-party binaries.

Notice sources:
- https://raw.githubusercontent.com/qt/qtbase/v6.8.3/LICENSES/LGPL-3.0-only.txt
- https://raw.githubusercontent.com/FFmpeg/FFmpeg/n7.1/COPYING.GPLv3
- https://raw.githubusercontent.com/mpv-player/mpv/e76a35ec95/Copyright
