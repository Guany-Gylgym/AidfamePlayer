#pragma once
#include <stdint.h>
#include <stddef.h>

/* C ABI for an optional, separately licensed offline camera decoder plugin.
 * All callbacks run on one dedicated decoding thread, never the GUI thread.
 * The host owns output buffers; the plugin never frees host memory.
 */
#define AIDFAME_DECODER_ABI_VERSION 1u
#define AIDFAME_DECODER_BGRA8 1u
#define AIDFAME_DECODER_AUDIO_FLOAT32 1u

#ifdef _WIN32
#define AIDFAME_PLUGIN_CALL __cdecl
#else
#define AIDFAME_PLUGIN_CALL
#endif

typedef struct AidfameDecoderInfoV1 {
    uint32_t struct_size;
    uint32_t width;
    uint32_t height;
    uint32_t fps_numerator;
    uint32_t fps_denominator;
    int64_t duration_us;
    uint32_t audio_rate;
    uint32_t audio_channels;
} AidfameDecoderInfoV1;

typedef struct AidfameVideoBufferV1 {
    uint32_t struct_size;
    uint32_t pixel_format;
    uint8_t* data;
    uint64_t capacity_bytes;
    uint32_t stride_bytes;
    int64_t presentation_time_us;
} AidfameVideoBufferV1;

typedef struct AidfameAudioBufferV1 {
    uint32_t struct_size;
    float* interleaved_samples;
    uint64_t capacity_frames;
    uint64_t written_frames;
    int64_t presentation_time_us;
} AidfameAudioBufferV1;

/* Return 0 on success, 1 at EOF, negative on failure. Error strings are UTF-8,
 * copied into the host-provided error buffer and always NUL-terminated.
 * open accepts a host-validated absolute local UTF-8 path, never a URL.
 */
typedef struct AidfameDecoderApiV1 {
    uint32_t struct_size;
    uint32_t abi_version;
    const char* plugin_name_utf8;
    int32_t (AIDFAME_PLUGIN_CALL *supports_extension)(const char* extension_utf8);
    int32_t (AIDFAME_PLUGIN_CALL *open)(const char* local_path_utf8, void** session,
        AidfameDecoderInfoV1* info, char* error_utf8, uint32_t error_capacity);
    int32_t (AIDFAME_PLUGIN_CALL *seek)(void* session, int64_t position_us);
    int32_t (AIDFAME_PLUGIN_CALL *read_video)(void* session, AidfameVideoBufferV1* output);
    int32_t (AIDFAME_PLUGIN_CALL *read_audio)(void* session, AidfameAudioBufferV1* output);
    void (AIDFAME_PLUGIN_CALL *cancel)(void* session);
    void (AIDFAME_PLUGIN_CALL *close)(void* session);
} AidfameDecoderApiV1;

/* Exported symbol: AidfameDecoderGetApi. Pointer remains valid until DLL unload.
 * Host must validate version, struct sizes and required function pointers.
 */
typedef const AidfameDecoderApiV1* (AIDFAME_PLUGIN_CALL *AidfameDecoderGetApiFn)(uint32_t host_abi_version);
