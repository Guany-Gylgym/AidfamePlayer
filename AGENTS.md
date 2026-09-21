# Aidfame Player Project Guide

## Scope

This is an independent C++ desktop application repository.

- Product: Aidfame Player, Chinese-first offline Windows desktop video player.
- Stack: C++17, Qt 6 Widgets, libmpv, FFmpeg, CMake, NSIS, JSON.
- Target: Windows 10/11 x64. No login, telemetry, update checks, or network media.
- UI text is Chinese; source, comments, commit messages, and technical docs are English.

## Workflow

- Read MEMORY.md and docs before implementation.
- Use karpathy-guidelines and ui-design-handoff-fidelity when those skills are available and applicable.
- Read docs/Developer-Guide.md and docs/Public-Testing.md. Build, run, fix, and commit focused changes separately.
- Never claim playback, hardware support, installer readiness, or soak-test success without runtime evidence.
- Never add private development history or raw internal test logs to this repository.
- CMake is the source of truth for Visual Studio projects.

## Safety

- Do not read secrets or credential files.
- Accept local regular files only; reject URLs and network shares.
- Disable external mpv configuration, scripts, playlist expansion, and network protocols before opening media.
- Cache deletion must remain inside the application cache root and must not traverse reparse points.
- Never delete user media, screenshots, or recordings during cache cleanup or uninstall.
- Use atomic JSON writes and validate settings at the boundary.

## Build

Use scripts/build.ps1 for the installed MSVC and project-local Qt SDK. Runtime dependencies are excluded from Git.
Tests use Qt Test and CTest. UI smoke tests run on the Windows Qt platform plugin; offscreen tests alone are not visual acceptance.
