# Optional BRAW decoder boundary

`src/Player/DecoderPluginApi.h` defines a versioned C ABI for an official-SDK
decoder. It does not pretend that mpv stream callbacks accept decoded AVFrames.
The bundled standalone FFmpeg 7.1 decoder listing has no BRAW decoder. The exact
libmpv FFmpeg capability still needs independent runtime inspection.

Host integration contract:

- Load an explicitly installed plugin only from `<application>/plugins/decoders`.
  Reject reparse points, unsupported ABI versions and short structs. Use
  LoadLibraryEx with DLL-load-directory/System32-only dependency search flags.
- Never search PATH, the media directory, a network location, or current folder.
- Plugin open/read/seek/close run on one bounded worker, cancel is thread-safe.
  Wait for the worker to finish before unloading the DLL. Failed/partial opens
  must leave the session null or closeable.
- Host provides capacity-checked BGRA frame and interleaved float audio buffers.
  Timestamps are signed microseconds on a single media timeline. The host handles
  A/V scheduling, speed, screenshots and recording; GPU frames require a future
  ABI version rather than casting pointers across vendors.
- No exceptions, STL objects, Qt objects, allocator ownership or C++ vtables
  cross the ABI. Failed calls must not change output capacity or ownership.
- A production host requires a mock plugin test matrix (ABI mismatch, short
  frame, EOF, cancellation, seek, allocation failure), plus licensed SDK builds
  and real BRAW fixtures before enabling playback.

Current status: interface design provided, runtime BRAW support NOT installed.
The normal player reports this limitation rather than trying another container
decoder or executing a DLL beside an untrusted video.
