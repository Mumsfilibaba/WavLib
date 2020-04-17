#include "WaveFormat.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int LoadWaveData(FILE* File, WaveHeader* Header, uint8_t** OutputBuffer)
{
    uint32_t SampleCount = (8 * Header->DataSize) / (Header->Channels * Header->BitsPerSample);
    uint32_t SampleSize = (Header->Channels * Header->BitsPerSample) / 8;

    // Make sure that the bytes-per-sample is completely divisible by num.of channels
    uint32_t BytesInEachChannel = (SampleSize / Header->Channels);
    if ((BytesInEachChannel * Header->Channels) != SampleSize)
    {
        printf("Error: %u * %u != %u\n", BytesInEachChannel, Header->Channels, SampleSize);
        return WAVE_ERROR;
    }

    if (Header->FormatType != 1)
    {
        printf("Error: Unsupported format = %u\n", Header->FormatType);
        return WAVE_ERROR;
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
            return WAVE_ERROR;
    }		

    printf("Low: %d, High: %d\n", LowLimit, HighLimit);

    const uint32_t BufferSize = SampleSize * SampleCount;
    (*OutputBuffer) = (uint8_t*)malloc(BufferSize);
    int32_t Read = fread((*OutputBuffer), 1, BufferSize, File);
    
    printf("Bytes read: %d, SizeOfStream: %u\n", Read, BufferSize);

    return WAVE_SUCCESS;
}

static int32_t LoadWaveFile(const char* Filename, uint8_t** OutputBuffer, WaveHeader* Header)
{
    FILE* File = fopen(Filename, "rb");
    if (File != NULL)
    {
        memset(Header, 0, sizeof(WaveHeader));

        // Riff
        int32_t Read = fread(&Header->RIFF, 1, sizeof(Header->RIFF), File);
        printf("Read=%d, Riff Marker=%s\n", Read, Header->RIFF);

        // Size
        uint8_t Buffer[4];
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->SizeInBytes = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, Size: %u bytes, %u kB\n", Read, Header->SizeInBytes, Header->SizeInBytes / 1024);

        // Wave
        Read = fread(Header->WAVE, 1, sizeof(Header->WAVE), File);
        printf("Read=%d, Wave Marker=%s\n", Read, Header->WAVE);

        // Format
        Read = fread(Header->FormatChunkMarker, 1, sizeof(Header->FormatChunkMarker), File);
        printf("Read=%d, FormatChunkMarker=%s\n", Read, Header->FormatChunkMarker);

        //Format size
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->FormatLength = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, FormatLength: %u bytes, %u kB\n", Read, Header->FormatLength, Header->FormatLength / 1024);

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
        printf("Read=%d, Format type: %u, Format name: %s\n", Read, Header->FormatType, FormatName);

        // Channels
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 2, File);
        Header->Channels = Buffer[0] | (Buffer[1] << 8);
        printf("Read=%d, Channels: %u\n", Read, Header->Channels);

        // Sample Rate
        memset(Buffer, 0, sizeof(Buffer));

        Read = fread(Buffer, 1, 4, File);
        Header->SampleRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, Sample rate: %u\n", Read, Header->SampleRate);

        memset(Buffer, 0, sizeof(Buffer));

        // Byte Rate
        Read = fread(Buffer, 1, 4, File);
        Header->ByteRate = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24);
        printf("Read=%d, Byte Rate: %u, Bit Rate:%u\n", Read, Header->ByteRate, Header->ByteRate * 8);

        memset(Buffer, 0, sizeof(Buffer));

        // Alignment
        Read = fread(Buffer, 1, 2, File);
        Header->BlockAlignment = Buffer[0] | (Buffer[1] << 8);
        printf("Read=%d, Block Alignment: %u \n", Read, Header->BlockAlignment);

        memset(Buffer, 0, sizeof(Buffer));

        // Bits per sample
        Read = fread(Buffer, 1, 2, File);
        Header->BitsPerSample = Buffer[0] | (Buffer[1] << 8);
        printf("Read=%d, Bits per sample: %u \n", Read, Header->BitsPerSample);

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
            printf("Read=%d, ChunkID: %s \n", Read, DataChunkHeader);

            memset(Buffer, 0, sizeof(Buffer));

            Read = fread(Buffer, 1, 4, File);
            Header->DataSize = Buffer[0] | (Buffer[1] << 8) | (Buffer[2] << 16) | (Buffer[3] << 24 );
            printf("Read=%d, Size of chunk: %u \n", Read, Header->DataSize);

            if (strcmp(DataChunkHeader, "data") == 0)
            {
                Result = LoadWaveData(File, Header, OutputBuffer);
                Search = 0;
            }
            else
            {
                if (feof(File))
                {
                    Result = WAVE_ERROR;
                    Search = 0;

                    printf("File does no contain any data\n");
                }
                else
                {
                    fseek(File, Header->DataSize, SEEK_CUR);
                    printf("Skipping Chunk - %s\n", DataChunkHeader);
                }
                
            }
        }

        fclose(File);
        return Result;
    }
    else
    {
        return WAVE_ERROR;
    }
}

static void ConvertHeader(WaveFile* FileHeader, const WaveHeader* Header)
{
    FileHeader->BitsPerSample   = Header->BitsPerSample;
    FileHeader->ByteRate        = Header->ByteRate;
    FileHeader->ChannelCount    = Header->Channels;
    FileHeader->SampleRate      = Header->SampleRate;
    FileHeader->SizeInBytes     = Header->DataSize;

    FileHeader->SampleCount     = (8 * Header->DataSize) / (Header->Channels * Header->BitsPerSample);
    printf("SampleCount=%u\n", FileHeader->SampleCount);

    FileHeader->SampleSize      = (Header->Channels * Header->BitsPerSample) / 8;
    printf("Size per sample=%u bytes\n", FileHeader->SampleSize);

    FileHeader->Duration        = (float)Header->DataSize / (float)Header->ByteRate;
    printf("Duration in seconds=%.4f\n", FileHeader->Duration);
}

int32_t LoadWaveSoundBuffer(const char* Filename, uint8_t** OutputBuffer, WaveFile* Header)
{
    WaveHeader WavHeader = {};
    int32_t result = LoadWaveFile(Filename, OutputBuffer, &WavHeader);
    if (result == WAVE_SUCCESS)
    {
        ConvertHeader(Header, &WavHeader);
    }

    return result;
}

int32_t LoadWaveSoundBufferFloat(const char* Filename, float** OutputBuffer, WaveFile* Header)
{
    WaveHeader  WavHeader   = {};
    uint8_t*    TempBuffer  = NULL;

    int32_t result = LoadWaveFile(Filename, &TempBuffer, &WavHeader);
    if (result == WAVE_SUCCESS)
    {
        ConvertHeader(Header, &WavHeader);

        uint32_t TotalSampleCount = Header->SampleCount * Header->ChannelCount;
        (*OutputBuffer)     = (float*)malloc(TotalSampleCount * sizeof(float));
        float* OutputIter   = (*OutputBuffer);

        if (WavHeader.BitsPerSample == 8)
        {
            int8_t* End = (int8_t*)(TempBuffer) + TotalSampleCount;
            for (int8_t* Begin = (int8_t*)TempBuffer; Begin != End;)
            {
                *(OutputIter) = (float)(*Begin) / 128.0f;

                OutputIter++;
                Begin++;
            }
        }
        else if (WavHeader.BitsPerSample == 16)
        {
            int16_t* End = (int16_t*)(TempBuffer) + TotalSampleCount;
            for (int16_t* Begin = (int16_t*)TempBuffer; Begin != End; )
            {
                *(OutputIter) = (float)(*Begin) / 32768.0f;
                
                OutputIter++;
                Begin++;
            }
        }
        else if (WavHeader.BitsPerSample == 32)
        {
            int32_t* End = (int32_t*)(TempBuffer) + TotalSampleCount;
            for (int32_t* Begin = (int32_t*)TempBuffer; Begin != End; )
            {
                *(OutputIter) = (float)(*Begin) / 2147483648.0f;

                OutputIter++;
                Begin++;
            }
        }

        free(TempBuffer);
    }

    return result;
}