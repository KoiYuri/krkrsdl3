#pragma once
#include "TVPWaveManager.h"

class FFWaveDecoderCreator : public tTVPWaveDecoderCreator
{
public:
    tTVPWaveDecoder* Create(const ttstr& storagename, const ttstr& extension);
};
