#include "WaveFormatConverter.h"

//---------------------------------------------------------------------------
// CPU specific optimized routine prototypes
//---------------------------------------------------------------------------
/**
 * int16→float32変換
 */
static void PCMConvertLoopInt16ToFloat32_c(void* dest, const void* src, size_t numsamples)
{
    float* d = static_cast<float*>(dest);
    const tjs_int16* s = static_cast<const tjs_int16*>(src);
    const tjs_int16* s_lim = s + numsamples;

    while (s < s_lim)
    {
        *d = *s * (1.0f / 32768.0f);
        d += 1;
        s += 1;
    }
}
//---------------------------------------------------------------------------
/**
 * float32→int16変換
 */
static void PCMConvertLoopFloat32ToInt16_c(void* dest, const void* src, size_t numsamples)
{
    tjs_uint16* d = static_cast<tjs_uint16*>(dest);
    const float* s = static_cast<const float*>(src);
    const float* s_lim = s + numsamples;

    while (s < s_lim)
    {
        float v = *s * 32767.0f;
        *d = v > (float)32767    ? 32767
             : v < (float)-32768 ? -32768
             : v < 0             ? (tjs_int16)(v - 0.5)
                                 : (tjs_int16)(v + 0.5);
        d += 1;
        s += 1;
    }
}
//---------------------------------------------------------------------------

PCMConvertLoopBaseFun PCMConvertLoopInt16ToFloat32 = PCMConvertLoopInt16ToFloat32_c;
PCMConvertLoopBaseFun PCMConvertLoopFloat32ToInt16 = PCMConvertLoopFloat32ToInt16_c;

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Wave format convertion routines
//---------------------------------------------------------------------------
static void TVPConvertFloatPCMTo16bits(
    tjs_int16* output, const float* input, tjs_int channels, tjs_int count, bool downmix)
{
    // convert 32bit float to 16bit integer

    // float PCM is in range of +1.0 ... 0 ... -1.0
    // clip sample which is out of the range.

    if (!downmix)
    {
        tjs_int total = channels * count;
        PCMConvertLoopFloat32ToInt16(output, input, total);
    }
    else
    {
        float nc = 32768.0f / (float)channels;
        while (count--)
        {
            tjs_int n = channels;
            float t = 0;
            while (n--)
                t += *(input++) * nc;
            if (t > 0)
            {
                int i = (int)(t + 0.5);
                if (i > 32767)
                    i = 32767;
                *(output++) = (tjs_int16)i;
            }
            else
            {
                int i = (int)(t - 0.5);
                if (i < -32768)
                    i = -32768;
                *(output++) = (tjs_int16)i;
            }
        }
    }
}
//---------------------------------------------------------------------------
static void TVPConvertIntegerPCMTo16bits(tjs_int16* output,
                                         const void* input,
                                         tjs_int bytespersample,
                                         tjs_int validbits,
                                         tjs_int channels,
                                         tjs_int count,
                                         bool downmix)
{
    // convert integer PCMs to 16bit integer PCM
#define PROCESS_BY_CHANNELS \
    switch (channels) \
    { \
        case 2: \
            PROCESS(2); \
            break; \
        case 4: \
            PROCESS(4); \
            break; \
        case 8: \
            PROCESS(8); \
            break; \
        default: \
            PROCESS(channels); \
            break; \
    }

#if TJS_HOST_IS_BIG_ENDIAN
#define GET_24BIT(p) (p[2] + (p[1] << 8) + (p[0] << 16))
#else
#define GET_24BIT(p) (p[0] + (p[1] << 8) + (p[2] << 16))
#endif

    if (bytespersample == 1)
    {
        // here assumes that the input 8bit PCM has always 8bit valid data
        const tjs_int8* p = (tjs_int8*)input;
        if (!downmix || channels == 1)
        {
            tjs_int total = channels * count;
            while (total--)
                *(output++) = (tjs_int16)(((tjs_int) * (p++) - 0x80) * 0x100);
        }
        else
        {
#define PROCESS(channels) \
    while (count--) \
    { \
        tjs_int v = 0; \
        tjs_int n = channels; \
        while (n--) \
            v += (tjs_int16)(((tjs_int) * (p++) - 0x80) * 0x100); \
        v = v / channels; \
        *(output++) = (tjs_int16)v; \
    }
            PROCESS_BY_CHANNELS
#undef PROCESS
        }
    }
    else if (bytespersample == 2)
    {
        tjs_uint16 mask = ~((1 << (16 - validbits)) - 1);
        const tjs_int16* p = (const tjs_int16*)input;
        if (!downmix || channels == 1)
        {
            tjs_int total = channels * count;
            while (total--)
                *(output++) = (tjs_int16)(*(p++) & mask);
        }
        else
        {
#define PROCESS(channels) \
    while (count--) \
    { \
        tjs_int v = 0; \
        tjs_int n = channels; \
        while (n--) \
            v += (tjs_int16)(*(p++) & mask); \
        v = v / channels; \
        *(output++) = (tjs_int16)v; \
    }
            PROCESS_BY_CHANNELS
#undef PROCESS
        }
    }
    else if (bytespersample == 3)
    {
        tjs_uint32 mask = ~((1 << (24 - validbits)) - 1);
        const tjs_uint8* p = (const tjs_uint8*)input;

        if (!downmix || channels == 1)
        {
            tjs_int total = channels * count;
            while (total--)
            {
                tjs_int32 t = GET_24BIT(p);
                p += 3;
                t |= -(t & 0x800000); // extend sign
                t &= mask;            // apply mask
                t >>= 8;
                *(output++) = (tjs_int16)t;
            }
        }
        else
        {
#define PROCESS(channels) \
    while (count--) \
    { \
        tjs_int v = 0; \
        tjs_int n = channels; \
        while (n--) \
        { \
            tjs_int32 t = GET_24BIT(p); \
            p += 3; \
            t |= -(t & 0x800000); \
            t &= mask; \
            t >>= 8; \
            v += t; \
        } \
        v = v / channels; \
        *(output++) = (tjs_int16)v; \
    }
            PROCESS_BY_CHANNELS
#undef PROCESS
        }
    }
    else if (bytespersample == 4)
    {
        tjs_int32 mask = ~((1 << (32 - validbits)) - 1);
        const tjs_int32* p = (const tjs_int32*)input;
        if (!downmix || channels == 1)
        {
            tjs_int total = channels * count;
            while (total--)
                *(output++) = (tjs_int16)((*(p++) & mask) >> 16);
        }
        else
        {
#define PROCESS(channels) \
    while (count--) \
    { \
        tjs_int v = 0; \
        tjs_int n = channels; \
        while (n--) \
            v += (tjs_int16)((*(p++) & mask) >> 16); \
        v = v / channels; \
        *(output++) = (tjs_int16)v; \
    }
            PROCESS_BY_CHANNELS
#undef PROCESS
        }
    }

#undef PROCESS_BY_CHANNELS
#undef GET_24BIT
}
//---------------------------------------------------------------------------
void TVPConvertPCMTo16bits(tjs_int16* output,
                           const void* input,
                           tjs_int channels,
                           tjs_int bytespersample,
                           tjs_int bitspersample,
                           bool isfloat,
                           tjs_int count,
                           bool downmix)
{
    // cconvert specified format to 16bit PCM

    if (isfloat)
        TVPConvertFloatPCMTo16bits(output, (const float*)input, channels, count, downmix);
    else
        TVPConvertIntegerPCMTo16bits(output, input, bytespersample, bitspersample, channels, count,
                                     downmix);
}
//---------------------------------------------------------------------------
void TVPConvertPCMTo16bits(
    tjs_int16* output, const void* input, const tTVPWaveFormat& format, tjs_int count, bool downmix)
{
    // cconvert specified format to 16bit PCM
    TVPConvertPCMTo16bits(output, input, format.Channels, format.BytesPerSample,
                          format.BitsPerSample, format.IsFloat, count, downmix);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
static void TVPConvertFloatPCMToFloat(float* output,
                                      const float* input,
                                      tjs_int channels,
                                      tjs_int count)
{
    // convert 32bit float to float

    // yes, acctually, this does nothing.
    memcpy(output, input, sizeof(float) * channels * count);
}
//---------------------------------------------------------------------------
static void TVPConvertIntegerPCMToFloat(float* output,
                                        const void* input,
                                        tjs_int bytespersample,
                                        tjs_int validbits,
                                        tjs_int channels,
                                        tjs_int count)
{
    // convert integer PCMs to float PCM

#ifdef TJS_HOST_IS_BIG_ENDIAN
#undef TJS_HOST_IS_BIG_ENDIAN
#endif
#if TJS_HOST_IS_BIG_ENDIAN
#define GET_24BIT(p) (p[2] + (p[1] << 8) + (p[0] << 16))
#else
#define GET_24BIT(p) (p[0] + (p[1] << 8) + (p[2] << 16))
#endif

    if (bytespersample == 1)
    {
        // here assumes that the input 8bit PCM has always 8bit valid data
        const tjs_int8* p = (tjs_int8*)input;
        tjs_int total = channels * count;
        while (total--)
            *(output++) = (float)(((tjs_int) * (p++) - 0x80) * (1.0 / 128));
    }
    else if (bytespersample == 2)
    {
        const tjs_int16* p = (const tjs_int16*)input;
        tjs_int total = channels * count;

        if (validbits == 16)
        {
            PCMConvertLoopInt16ToFloat32(output, p, total);
        }
        else
        {
            // generic
            tjs_uint16 mask = ~((1 << (16 - validbits)) - 1);

            while (total--)
                *(output++) = (float)((*(p++) & mask) * (1.0 / 32768));
        }
    }
    else if (bytespersample == 3)
    {
        tjs_uint32 mask = ~((1 << (24 - validbits)) - 1);
        const tjs_uint8* p = (const tjs_uint8*)input;

        tjs_int total = channels * count;
        while (total--)
        {
            tjs_int32 t = GET_24BIT(p);
            p += 3;
            t |= -(t & 0x800000); // extend sign
            t &= mask;            // apply mask
            *(output++) = (float)(t * (1.0 / (1 << 23)));
        }
    }
    else if (bytespersample == 4)
    {
        tjs_int32 mask = ~((1 << (32 - validbits)) - 1);
        const tjs_int32* p = (const tjs_int32*)input;
        tjs_int total = channels * count;
        while (total--)
            *(output++) = (float)(((*(p++) & mask) >> 0) * (1.0 / (1 << 31)));
    }
}
//---------------------------------------------------------------------------
void TVPConvertPCMToFloat(float* output,
                          const void* input,
                          tjs_int channels,
                          tjs_int bytespersample,
                          tjs_int bitspersample,
                          bool isfloat,
                          tjs_int count)
{
    // cconvert specified format to 16bit PCM

    if (isfloat)
        TVPConvertFloatPCMToFloat(output, (const float*)input, channels, count);
    else
        TVPConvertIntegerPCMToFloat(output, input, bytespersample, bitspersample, channels, count);
}
//---------------------------------------------------------------------------
void TVPConvertPCMToFloat(float* output,
                          const void* input,
                          const tTVPWaveFormat& format,
                          tjs_int count)
{
    // cconvert specified format to 16bit PCM
    TVPConvertPCMToFloat(output, input, format.Channels, format.BytesPerSample,
                         format.BitsPerSample, format.IsFloat, count);
}
//---------------------------------------------------------------------------