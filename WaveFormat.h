#include <stdint.h>

#define WAVE_ERROR   (int)-1
#define WAVE_SUCCESS (int)0

#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_IEEE_FLOAT  0x0003
#define WAVE_FORMAT_ALAW        0x0006
#define WAVE_FORMAT_MULAW       0x0007
#define WAVE_FORMAT_EXTENSIBLE  0xffff

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
int32_t LoadWaveSoundBuffer(const char* Filename, uint8_t** OutputBuffer, WaveFile* Header);

/*
    Returns a soundbuffer from .wav file as a normalized floating-point buffer.
    Caller is responsible for calling free.
*/
int32_t LoadWaveSoundBufferFloat(const char* Filename, float** OutputBuffer, WaveFile* Header);