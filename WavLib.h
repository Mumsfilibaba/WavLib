#ifndef WAVLIB_H
#define WAVLIB_H

#include <stdint.h>

#define WAVE_NO_DATA                    (int32_t)-4
#define WAVE_FILE_NOT_FOUND             (int32_t)-3
#define WAVE_UNSUPPORTED_FORMAT         (int32_t)-2
#define WAVE_CORRUPT_FILE               (int32_t)-1
#define WAVE_SUCCESS                    (int32_t)0

#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_IEEE_FLOAT  0x0003
#define WAVE_FORMAT_ALAW        0x0006
#define WAVE_FORMAT_MULAW       0x0007
#define WAVE_FORMAT_EXTENSIBLE  0xffff

#ifdef __cplusplus
extern "C"
{
#endif

enum WavLibFlags
{
    WAV_LIB_FLAG_NONE = 0,
    WAV_LIB_FLAG_MONO = 0x01,
};

typedef struct tagWaveHeader
{
    char        RIFF[4];
    uint32_t    SizeInBytes;
    char        WAVE[4];
    char        FormatChunkMarker[4];
    uint32_t    FormatLength;
    uint16_t    FormatType;
    uint16_t    Channels;
    uint32_t    SampleRate;
    uint32_t    ByteRate;
    uint16_t    BlockAlignment;
    uint16_t    BitsPerSample;
    char        DataChunkHeader[4];
    uint32_t    DataSize;
} WaveHeader;

typedef struct tagWaveFile
{
    uint32_t    SizeInBytes;
    uint32_t    TotalSampleCount;
    uint32_t    SampleCount;
    uint32_t    SampleSize;
    uint32_t    SampleRate;
    uint32_t    ByteRate;
    uint32_t    BitsPerSample;
    uint32_t    ChannelCount;
    float       Duration;
} WaveFile;

/*
    Returns a soundbuffer from .wav file as a byte buffer.
    Caller is responsible for calling free.
*/
int32_t WavLibLoadFile(const char* Filename, uint8_t** OutputBuffer, WaveFile* Header, uint32_t Flags);

/*
    Returns a soundbuffer from .wav file as a normalized 32-bit floating-point buffer.
    Caller is responsible for calling free.
*/
int32_t WavLibLoadFileFloat32(const char* Filename, float** OutputBuffer, WaveFile* Header, uint32_t Flags);

/*
    Returns a soundbuffer from .wav file as a normalized 64-bit floating-point buffer.
    Caller is responsible for calling free.
*/
int32_t WavLibLoadFileFloat64(const char* Filename, double** OutputBuffer, WaveFile* Header, uint32_t Flags);

void*   WavLibMalloc(uint32_t Size);
void    WavLibFree(void* Buffer);

const char* WavLibGetError(int32_t ErrorCode);

#ifdef __cplusplus
}
#endif
#endif