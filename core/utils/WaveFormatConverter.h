#pragma once

#include "tjsCommHead.h"

/*[*/
//---------------------------------------------------------------------------
// PCM data format (internal use)
//---------------------------------------------------------------------------
struct tTVPWaveFormat
{
    tjs_uint SamplesPerSec; // sample granule per sec
    tjs_uint Channels;
    tjs_uint BitsPerSample;   // per one sample
    tjs_uint BytesPerSample;  // per one sample
    tjs_uint64 TotalSamples;  // in sample granule; unknown for zero
    tjs_uint64 TotalTime;     // in ms; unknown for zero
    tjs_uint32 SpeakerConfig; // bitwise OR of SPEAKER_* constants
    bool IsFloat;             // true if the data is IEEE floating point
    bool Seekable;
};
//---------------------------------------------------------------------------

/*]*/
//---------------------------------------------------------------------------
// PCM bit depth converter
//---------------------------------------------------------------------------
extern void TVPConvertPCMTo16bits(tjs_int16* output,
                                  const void* input,
                                  const tTVPWaveFormat& format,
                                  tjs_int count,
                                  bool downmix);
extern void TVPConvertPCMTo16bits(tjs_int16* output,
                                  const void* input,
                                  tjs_int channels,
                                  tjs_int bytespersample,
                                  tjs_int bitspersample,
                                  bool isfloat,
                                  tjs_int count,
                                  bool downmix);
extern void TVPConvertPCMToFloat(float* output,
                                 const void* input,
                                 tjs_int channels,
                                 tjs_int bytespersample,
                                 tjs_int bitspersample,
                                 bool isfloat,
                                 tjs_int count);
extern void TVPConvertPCMToFloat(float* output,
                                 const void* input,
                                 const tTVPWaveFormat& format,
                                 tjs_int count);
//---------------------------------------------------------------------------
typedef void (*PCMConvertLoopBaseFun)(void* dest, const void* src, size_t numsamples);
extern PCMConvertLoopBaseFun PCMConvertLoopInt16ToFloat32;
extern PCMConvertLoopBaseFun PCMConvertLoopFloat32ToInt16;
//---------------------------------------------------------------------------