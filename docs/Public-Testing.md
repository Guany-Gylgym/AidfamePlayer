# Test scope for the source release

The internal 1.0.0 build was tested on one Windows 11 workstation with an NVIDIA
RTX 5070 Laptop GPU and AMD Radeon 890M. Results describe that build and host,
not every build from this repository or every GPU.

- Debug and Release: six CTest groups passed.
- Native UI checks at three DPI settings passed.
- All 96 private camera clips played to EOF after an EOF-state fix.
- A separate native playback-core soak ran uninterrupted for 7200.09 seconds,
  28 loops and 240 samples, with a finished passing report and process exit 0.
- Screenshot, recording AV-sync, cache junction and installer-harness checks passed.
- Internal production installation passed 39 payload hashes, shortcut checks
  and SDK-independent startup/exit.

Private media, raw logs, personal paths and internal Git history are excluded.
These are maintainer-reported results, not independent public CI results.
Synthetic fixtures can be regenerated with the included scripts.

Unverified/incomplete: Windows 10, Intel/QSV, some camera profiles, disk-full
and cache TOCTOU fault injection. BRAW runtime decoding is not implemented.
8K incurred dropped frames under load. The two-hour run covers the native
playback core, not every UI/recording workflow for two hours.

Rerun relevant tests after source/dependency changes. Public binaries require
the separate compliance gate in Dependencies.md.
