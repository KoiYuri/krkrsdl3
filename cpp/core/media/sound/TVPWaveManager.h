//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Wave Player interface
//---------------------------------------------------------------------------
#ifndef WaveIntfH
#define WaveIntfH

#include "WaveSegmentQueue.h"

#include "tjsCommHead.h"
#include "tjsNative.h"
#include "tjsUtils.h"
#include "WaveFormatConverter.h"

//---------------------------------------------------------------------------
// tTVPWaveDecoder interface
//---------------------------------------------------------------------------
class tTVPWaveDecoder
{
public:
    virtual ~tTVPWaveDecoder(){};

    virtual void GetFormat(tTVPWaveFormat& format) = 0;
    /* Retrieve PCM format, etc. */

    virtual bool Render(void* buf, tjs_uint bufsamplelen, tjs_uint& rendered) = 0;
    /*
            Render PCM from current position.
            where "buf" is a destination buffer, "bufsamplelen" is the buffer's
            length in sample granule, "rendered" is to be an actual number of
            written sample granule.
            returns whether the decoding is to be continued.
            because "redered" can be lesser than "bufsamplelen", the player
            should not end until the returned value becomes false.
    */

    virtual bool SetPosition(tjs_uint64 samplepos) = 0;
    /*
            Seek to "samplepos". "samplepos" must be given in unit of sample granule.
            returns whether the seeking is succeeded.
    */

    virtual bool DesiredFormat(const tTVPWaveFormat& format) { return false; }
};
//---------------------------------------------------------------------------
class tTVPWaveDecoderCreator
{
public:
    virtual tTVPWaveDecoder* Create(const ttstr& storagename, const ttstr& extension) = 0;
    /*
            Create tTVPWaveDecoder instance. returns NULL if failed.
    */
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPWaveDecoder interface management
//---------------------------------------------------------------------------
extern void TVPRegisterWaveDecoderCreator(tTVPWaveDecoderCreator* d);
extern void TVPUnregisterWaveDecoderCreator(tTVPWaveDecoderCreator* d);
extern tTVPWaveDecoder* TVPCreateWaveDecoder(const ttstr& storagename);
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// interface for basic filter management
//---------------------------------------------------------------------------
class tTVPSampleAndLabelSource;
class iTVPBasicWaveFilter
{
public:
    // recreate filter. filter will remain owned by the each filter instance.
    virtual tTVPSampleAndLabelSource* Recreate(tTVPSampleAndLabelSource* source) = 0;
    virtual void Clear(void) = 0;
    virtual void Update(void) = 0;
    virtual void Reset(void) = 0;
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Constants
//---------------------------------------------------------------------------

#define TVP_WSB_ACCESS_FREQ (8) // wave sound buffer access frequency (hz)

#define TVP_TIMEOFS_INVALID_VALUE ((tjs_int)(-2147483648LL)) // invalid value for 32bit time offset

//---------------------------------------------------------------------------

extern void TVPReleaseDirectSound();
extern void TVPResetVolumeToAllSoundBuffer();
//---------------------------------------------------------------------------

#endif
