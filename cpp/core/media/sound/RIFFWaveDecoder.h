#pragma once

#include "TVPWaveManager.h"

//---------------------------------------------------------------------------
// default RIFF Wave Decoder creator
//---------------------------------------------------------------------------
class tTVPWDC_RIFFWave : public tTVPWaveDecoderCreator
{
public:
    tTVPWaveDecoder* Create(const ttstr& storagename, const ttstr& extension);
};