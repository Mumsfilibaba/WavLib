#include "WavLib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define WAVLIB_DEBUG_ENABLED 0
#if WAVLIB_DEBUG_ENABLED
    #define DebugPrint(...) printf(__VA_ARGS__)
#else
    #define DebugPrint(...)
#endif

static int _WavLibLoadData(FILE* File, WaveHeader* Header, uint8_t** OutputBuffer)
{
    uint32_t SampleCount = (8 * Header->DataSize) / (Header->Channels * Header->BitsPerSample);
    uint32_t SampleSize = (Header->Channels * Header->BitsPerSample) / 8;

    // Make sure that the bytes-per-sample is completely divisible by number of channels
    uint32_t BytesInEachChannel = (SampleSize / Header->Channels);
    if ((BytesInEachChannel * Header->Channels) != SampleSize)
    {
        DebugPrint("Error: %u * %u != %u\n", BytesInEachChannel, Header->Channels, SampleSize);
        return WAVE_CORRUPT_FILE;
    }

    if (Header->FormatType != 1)
    {
        DebugPrint("Error: Unsupported format = %u\n", Header->FormatType);
        return WAVE_UNSUPPORTED_FORMAT;
    }

    int32_t LowLimit   = 0;
    int32_t HighLimit  = 0;
    switch (Header->BitsPerSample) 
    {
        case 8:
            LowLimit    = -128;
            HighLimit   = 127;
            break;
        case 16:
            LowLimit    = -32768;
            HighLimit   = 32767;
            break;
        case 32:
            LowLimit    = -2147483648;
            HighLimit   = 2147483647;
            break;
        default:
            printf("Error: Unsupported BitsPerSample=%u\n", Header->BitsPerSample);
            return WAVE_UNSUPPORTED_FORMAT;
    }		

    DebugPrint("Low: %d, High: %d\n", LowLimit, HighLimit);

    const uint32_t BufferSize = SampleSize * SampleCount;
    (*OutputBuffer) = (uint8_t*)WavLibMalloc(BufferSize);
    int32_t Read = fread((*OutputBuffer), 1, BufferSize, File);
    
    DebugPrint("Bytes read: %d, SizeOfStream: %u\n", Read, BufferSize);

    return WAVE_SUCCESS;
}

static int32_t _WavLibLoadFile(const char* Filename, uint8_t** OutputBuffer, WaveHeader* Header)
{
    FILE* File = fopen(Filename, "rb");
    if (File != NULL)
    {
        memset(Header, 0, sizeof(WaveHeader));

        // Riff
        int32_t Read = fread(&Header->RIFF, 1, sizeof(Header->RIFF), File);
        
        DebugPrint("Read=%d, Riff Marker=%s\n", Read, Header->RIFF);

        // Size
        uint8_t Buffer[4];
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->SizeInBytes = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        
        DebugPrint("Read=%d, Size: %u bytes, %u kB\n", Read, Header->SizeInBytes, Header->SizeInBytes / 1024);

        // Wave
        Read = fread(Header->WAVE, 1, sizeof(Header->WAVE), File);
        
        DebugPrint("Read=%d, Wave Marker=%s\n", Read, Header->WAVE);

        // Format
        Read = fread(Header->FormatChunkMarker, 1, sizeof(Header->FormatChunkMarker), File);
        
        DebugPrint("Read=%d, FormatChunkMarker=%s\n", Read, Header->FormatChunkMarker);

        //Format size
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->FormatLength = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        
        DebugPrint("Read=%d, FormatLength: %u bytes, %u kB\n", Read, Header->FormatLength, Header->FormatLength / 1024);

        // Format type
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->FormatType = Buffer[0] | (Buffer[1] << 8);

        const char* FormatName = "";
        if (Header->FormatType == WAVE_FORMAT_PCM)
        {
            FormatName = "PCM";
        }
        else if (Header->FormatType == WAVE_FORMAT_IEEE_FLOAT)
        {
            FormatName = "Float";
        }
        else if (Header->FormatType == WAVE_FORMAT_ALAW)
        {
            FormatName = "A-law";
        }
        else if (Header->FormatType == WAVE_FORMAT_MULAW)
        {
            FormatName = "Mu-law";
        }
        else if (Header->FormatType == WAVE_FORMAT_EXTENSIBLE)
        {
            FormatName = "Extensible";
        }

        DebugPrint("Read=%d, Format type: %u, Format name: %s\n", Read, Header->FormatType, FormatName);

        // Channels
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->Channels = Buffer[0] | (Buffer[1] << 8);
        
        DebugPrint("Read=%d, Channels: %u\n", Read, Header->Channels);

        // Sample Rate
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->SampleRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);

        DebugPrint("Read=%d, Sample rate: %u\n", Read, Header->SampleRate);

        memset(Buffer, 0, sizeof(Buffer));

        // Byte Rate
        Read = fread(Buffer, 1, 4, File);
        Header->ByteRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        
        DebugPrint("Read=%d, Byte Rate: %u, Bit Rate:%u\n", Read, Header->ByteRate, Header->ByteRate * 8);

        memset(Buffer, 0, sizeof(Buffer));

        // Alignment
        Read = fread(Buffer, 1, 2, File);
        Header->BlockAlignment = Buffer[0] | (Buffer[1] << 8);
        
        DebugPrint("Read=%d, Block Alignment: %u \n", Read, Header->BlockAlignment);

        memset(Buffer, 0, sizeof(Buffer));

        // Bits per sample
        Read = fread(Buffer, 1, 2, File);
        Header->BitsPerSample = Buffer[0] | (Buffer[1] << 8);

        DebugPrint("Read=%d, Bits per sample: %u \n", Read, Header->BitsPerSample);

        // If format chunk is bigger we skip the extra bytes
        uint32_t FormatSize = Header->FormatLength - 16;
        if (FormatSize != 0)
        {
            fseek(File, FormatSize, SEEK_CUR);
        }

        char DataChunkHeader[5];
        int32_t Result = 0;
        int32_t Search = 1;
        while (Search != 0)
        {
            // Data type
            Read = fread(Header->DataChunkHeader, 1, sizeof(Header->DataChunkHeader), File);
            memset(DataChunkHeader, 0, sizeof(DataChunkHeader));
            memcpy(DataChunkHeader, Header->DataChunkHeader, sizeof(Header->DataChunkHeader));

            DebugPrint("Read=%d, ChunkID: %s \n", Read, DataChunkHeader);

            memset(Buffer, 0, sizeof(Buffer));

            Read = fread(Buffer, 1, 4, File);
            Header->DataSize = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24 );

            DebugPrint("Read=%d, Size of chunk: %u \n", Read, Header->DataSize);

            if (strcmp(DataChunkHeader, "data") == 0)
            {
                Result = _WavLibLoadData(File, Header, OutputBuffer);
                Search = 0;
            }
            else
            {
                if (feof(File))
                {
                    Result = WAVE_NO_DATA;
                    Search = 0;

                    DebugPrint("File does no contain any data\n");
                }
                else
                {
                    fseek(File, Header->DataSize, SEEK_CUR);
                    DebugPrint("Skipping Chunk - %s\n", DataChunkHeader);
                }
                
            }
        }

        fclose(File);
        return Result;
    }
    else
    {
        return WAVE_FILE_NOT_FOUND;
    }
}

static void _WavLibConvertHeader(WaveFile* FileHeader, const WaveHeader* Header)
{
    FileHeader->BitsPerSample   = Header->BitsPerSample;
    FileHeader->ByteRate        = Header->ByteRate;
    FileHeader->ChannelCount    = Header->Channels;
    FileHeader->SampleRate      = Header->SampleRate;
    FileHeader->SizeInBytes     = Header->DataSize;

    FileHeader->TotalSampleCount = (8 * Header->DataSize) / Header->BitsPerSample;
    
    DebugPrint("SampleCount=%u\n", FileHeader->TotalSampleCount);

    FileHeader->SampleCount = FileHeader->TotalSampleCount / FileHeader->ChannelCount;

    FileHeader->SampleSize      = (Header->Channels * Header->BitsPerSample) / 8;
    
    DebugPrint("Size per sample=%u bytes\n", FileHeader->SampleSize);

    FileHeader->Duration        = (float)Header->DataSize / (float)Header->ByteRate;

    DebugPrint("Duration in seconds=%.4f\n", FileHeader->Duration);
}

static void _WavLibCombineChannels(const uint8_t* InputBuffer, uint8_t** OutputBuffer, uint32_t BitsPerSample, uint32_t SampleCount, uint32_t ChannelCount)
{
    const uint32_t BytesPerSample   = BitsPerSample / 8;
    const uint32_t TotalSizeInBytes = BytesPerSample * SampleCount * ChannelCount;

    uint8_t* Output = (uint8_t*)WavLibMalloc(TotalSizeInBytes);
    memset(Output, 0, TotalSizeInBytes);

    if (BytesPerSample == 1)
    {
        int8_t*        Out    = (int8_t*)Output;
        const int8_t*  In     = (const int8_t*)InputBuffer;
        for (uint32_t s = 0; s < SampleCount; s++)
        {
            int32_t sample = 0;
            for (uint32_t c = 0; c < ChannelCount; c++)
            {
                sample += (int32_t)(*In);
                (*Out) += (*In);
                In++;  
            }

            (*Out) = (int8_t)(sample / ChannelCount);
            Out++;
        }
    }
    else if (BytesPerSample == 2)
    {
        int16_t*        Out    = (int16_t*)Output;
        const int16_t*  In     = (const int16_t*)InputBuffer;
        for (uint32_t s = 0; s < SampleCount; s++)
        {
            int64_t sample = 0;
            for (uint32_t c = 0; c < ChannelCount; c++)
            {
                sample += (int64_t)(*In);
                In++;
            }

            (*Out) = (int16_t)(sample / ChannelCount);
            Out++;
        }
    }
    else if (BytesPerSample == 4)
    {
        int32_t*        Out    = (int32_t*)Output;
        const int32_t*  In     = (const int32_t*)InputBuffer;
        for (uint32_t s = 0; s < SampleCount; s++)
        {
            int64_t sample = 0;
            for (uint32_t c = 0; c < ChannelCount; c++)
            {
                sample += (int64_t)(*In);
                In++;  
            }

            (*Out) = (int32_t)(sample / ChannelCount);
            Out++;
        }
    }

    (*OutputBuffer) = Output;
}

static void _WavLibPreProcess(const uint8_t* In, uint8_t** Out, WaveFile* Header, uint32_t Flags)
{
    if (Flags & WAV_LIB_FLAG_MONO)
    {
        _WavLibCombineChannels(In, Out, Header->BitsPerSample, Header->SampleCount, Header->ChannelCount);

        // Make sure that the channel count gets updated
        Header->ChannelCount = 1;
        
        // Free the old storage
        WavLibFree(In);
    }
    else
    {
        (*Out) = In;
    }
}

int32_t WavLibLoadFile(const char* Filename, uint8_t** OutputBuffer, WaveFile* Header, uint32_t Flags)
{
    uint8_t* TempOut = NULL;

    WaveHeader  WavHeader;
    int32_t     Result = _WavLibLoadFile(Filename, &TempOut, &WavHeader);
    if (Result == WAVE_SUCCESS)
    {
        _WavLibConvertHeader(Header, &WavHeader);
        _WavLibPreProcess(TempOut, OutputBuffer, Header, Flags);
    }

    return Result;
}

int32_t WavLibLoadFileFloat(const char* Filename, float** OutputBuffer, WaveFile* Header, uint32_t Flags)
{
    WaveHeader  WavHeader;
    uint8_t*    FileData  = NULL;

    int32_t Result = _WavLibLoadFile(Filename, &FileData, &WavHeader);
    if (Result == WAVE_SUCCESS)
    {
        _WavLibConvertHeader(Header, &WavHeader);

        uint8_t* TempOut = NULL;
        _WavLibPreProcess(FileData, &TempOut, Header, Flags);

        // Convert to floating point
        uint32_t TotalSampleCount = Header->SampleCount * Header->ChannelCount;
        (*OutputBuffer)     = (float*)WavLibMalloc(TotalSampleCount * sizeof(float));
        float* OutputIter   = (*OutputBuffer);

        if (WavHeader.BitsPerSample == 8)
        {
            int8_t* End = (int8_t*)(TempOut) + TotalSampleCount;
            for (int8_t* Begin = (int8_t*)TempOut; Begin != End;)
            {
                *(OutputIter) = (float)(*Begin) / 128.0f;

                OutputIter++;
                Begin++;
            }
        }
        else if (WavHeader.BitsPerSample == 16)
        {
            int16_t* End = (int16_t*)(TempOut) + TotalSampleCount;
            for (int16_t* Begin = (int16_t*)TempOut; Begin != End; )
            {
                *(OutputIter) = (float)(*Begin) / 32768.0f;
                
                OutputIter++;
                Begin++;
            }
        }
        else if (WavHeader.BitsPerSample == 32)
        {
            int32_t* End = (int32_t*)(TempOut) + TotalSampleCount;
            for (int32_t* Begin = (int32_t*)TempOut; Begin != End; )
            {
                *(OutputIter) = (float)(*Begin) / 2147483648.0f;

                OutputIter++;
                Begin++;
            }
        }

        WavLibFree(TempOut);
    }

    return Result;
}

void* WavLibMalloc(uint32_t Size)
{
    return malloc(Size);
}

void WavLibFree(void* Buffer)
{
    free(Buffer);
}

const char* WavLibGetError(int32_t ErrorCode)
{
    switch (ErrorCode)
    {
        case WAVE_NO_DATA:              return "File does not contain any data chunk";
        case WAVE_FILE_NOT_FOUND:       return "File not found";
        case WAVE_UNSUPPORTED_FORMAT:   return "Unsupported format";
        case WAVE_CORRUPT_FILE:         return "Corrupt file";
        case WAVE_SUCCESS:              return "Success";
        default:                        return "Unknown error";
    }
}