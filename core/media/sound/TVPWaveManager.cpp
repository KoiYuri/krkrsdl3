//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Wave Player interface
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "TVPWaveManager.h"
#include "TVPMsg.h"
#include "UtilStreams.h"

#include "RIFFWaveDecoder.h"

//---------------------------------------------------------------------------
// tTVPWaveDecoder interface management
//---------------------------------------------------------------------------
static bool TVPWaveDecoderManagerAvail = false;
struct tTVPWaveDecoderManager
{
    std::vector<tTVPWaveDecoderCreator*> Creators;
    tTVPWDC_RIFFWave RIFFWaveDecoderCreator;

    tTVPWaveDecoderManager()
    {
        TVPWaveDecoderManagerAvail = true;
        TVPRegisterWaveDecoderCreator(&RIFFWaveDecoderCreator);
    }

    ~tTVPWaveDecoderManager() { TVPWaveDecoderManagerAvail = false; }

} static TVPWaveDecoderManager;
//---------------------------------------------------------------------------
void TVPRegisterWaveDecoderCreator(tTVPWaveDecoderCreator* d)
{
    if (TVPWaveDecoderManagerAvail)
        TVPWaveDecoderManager.Creators.push_back(d);
}
//---------------------------------------------------------------------------
void TVPUnregisterWaveDecoderCreator(tTVPWaveDecoderCreator* d)
{
    if (TVPWaveDecoderManagerAvail)
    {
        std::vector<tTVPWaveDecoderCreator*>::iterator i;
        i = std::find(TVPWaveDecoderManager.Creators.begin(), TVPWaveDecoderManager.Creators.end(),
                      d);
        if (i != TVPWaveDecoderManager.Creators.end())
        {
            TVPWaveDecoderManager.Creators.erase(i);
        }
    }
}
//---------------------------------------------------------------------------
tTVPWaveDecoder* TVPCreateWaveDecoder(const ttstr& storagename)
{
    // find a decoder and create its instance.
    // throws an exception when the decodable decoder is not found.
    if (!TVPWaveDecoderManagerAvail)
        return NULL;

    ttstr ext(TVPExtractStorageExt(storagename));
    ext.ToLowerCase();

    // .wav first
    tTVPWaveDecoder* decoder = TVPWaveDecoderManager.Creators[0]->Create(storagename, ext);
    if (decoder)
        return decoder;

    // other
    tjs_int i = (tjs_int)(TVPWaveDecoderManager.Creators.size() - 1);
    for (; i >= 1; i--)
    {
        decoder = TVPWaveDecoderManager.Creators[i]->Create(storagename, ext);
        if (decoder)
            return decoder;
    }

    TVPThrowExceptionMessage(TVPUnknownWaveFormat, storagename);
    return NULL;
}
//---------------------------------------------------------------------------