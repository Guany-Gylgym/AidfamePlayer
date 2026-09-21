# Design Source of Truth

This specification and the editable Stage 1 Qt source define the static UI. The user supplied a cyan liquid play emblem, not a full-window screenshot. The original bitmap is currently unavailable; a clearly documented interim vector play mark is used only for development builds.

## Canvas and layout

Windows desktop, native window frame. Default client size 1280x800, minimum 800x500. Menus across the top: File, Playback, Settings (Chinese labels). Center is a quiet near-black video viewport. Bottom has a thin seek bar, then one compact control row. No dashboard cards or decorative panels around video. An empty-state centered brand mark, title, brief offline description, and Open Video button form the primary action. Recent history is empty initially.

## Tokens

| Token | Value |
|---|---|
| canvas | #101114 |
| viewport | #08090b |
| surface | #191b20 |
| surface-hover | #292d35 |
| border | #30343d |
| text | #eef0f4 |
| secondary | #9299a6 |
| disabled | #606773 |
| accent | #37c8ee |
| accent-pressed | #179dbc |
| font | Microsoft YaHei UI, 10pt |
| title | 22pt, semibold |
| time | Consolas, 10pt |
| spacing | 4, 8, 12, 16, 24 |
| radius | 5px controls, 8px primary button |
| menu-height | 30px |
| control-row | >=56px |
| icon | 20px in >=32px target |

## States and constraints

Static Stage 1: zero time, no fake media, original resolution, 1x speed, 80 volume. Playback, screenshot, recording, and seek controls disabled. Fullscreen and menu navigation work. File-open action explains the stage limitation until the backend is integrated. Small windows retain all essential controls. Tooltips and accessible names describe every icon; use SVG icons rather than text glyphs.

## QA

## Settings dialog extension

Mini-mode extension: 480x320 default, 360x300 minimum, native draggable/resizable
frame, always on top. Hide menu/status and secondary transport controls; keep
play/pause, time, seek and fullscreen. Ctrl+M or Escape restores normal mode.
The same native video child is retained; changing topmost must not recreate it.

Purpose: configure offline playback and storage without obscuring the current
video permanently. Industrial/utilitarian treatment using the established
black-gray tokens, cyan focus, Microsoft YaHei UI, native controls.
Canvas: 640x460 logical pixels, minimum 600x420; 16px outer margins, 12px form
spacing. Seven tabs: playback, shortcuts, screenshots, recording, cache,
performance, default apps. Text-only tab labels, no new decorative assets.
Forms stretch path inputs; native browse buttons stay visible. Bottom-right
Cancel/Save; destructive cache clearing is an explicit separate action.
First build with fixed values and no persistence bindings, capture native QA,
then connect SettingsStore. Long paths scroll horizontally, never widen dialog.


Check 1280x800 and 800x500 at 100%, and representative 150/200% DPI. Inspect structure, alignment, readable labels, stable SVG rendering, focus outline, and control visibility. Save screenshots as QA artifacts, but keep Qt source and SVG assets as the handoff. Do not attach real media state until Stage 1 screenshot QA passes.
