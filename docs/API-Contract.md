# Internal Contracts

## Playback

Commands: openLocalFile(path), setPaused(bool), seekRelative(seconds), seekAbsolute(seconds), setVolume(0..100), setSpeed(allowed value), setAspect(mode), setResolutionCap(mode), stop(). All commands report failure; no fabricated state transitions before backend acknowledgement.

Events: statusChanged, positionChanged, durationChanged, metadataChanged, decoderChanged, errorOccurred, fileEnded. Callbacks are delivered to the Qt main thread. Unknown duration and bitrate remain unknown rather than zero-valued facts.

## Settings schema v1

Arrow shortcut routing: in the player window, Left/Right seek by the selected
5/10/15/20/30 second step and Up/Down adjust volume by 5 points (clamped 0..100).
Menus, modal dialogs, text inputs, and focused combo boxes retain native keys.
Disabled playback never consumes these keys. The Settings > Seek step submenu
selects the step; persistence is integrated in Stage 7.

Fields: seekStepSeconds, screenshotFormat, screenshotDirectory, recordingDirectory, hardwareDecoder, showMediaInfo, volume, speed, aspectMode. Unknown keys are ignored; invalid known values fall back to documented defaults. Paths are validated at operation time.

## History schema v1

Entries contain path, lastPlayedUtc, positionSeconds, durationSeconds. A bounded list is sorted by lastPlayedUtc descending. Removing an entry affects only JSON. Missing files are reported without silently rewriting paths.

## Recording

States: idle -> starting -> recording -> stopping -> completed, with failure/cancellation branches. Completion occurs only after encoder exit success and playable output validation. Existing files are never overwritten without confirmation. Recording metadata must retain capture semantics and selected speed/resolution.

## BRAW plugin (planned)

ABI version, capability query, open/close, metadata, seek, video frame ownership, audio buffer ownership, timestamps, cancellation, error reporting. The interface is finalized only after SDK prototype evidence. A declared interface alone does not mean BRAW playback is implemented.
