#pragma once

#include "WaveFormatConverter.h"
#include <atomic>

class iTVPDecodeSoundBuffer
{
public:
    std::atomic_bool DecodeActive{false};
    std::atomic_bool DecodeInProgress{false};
    std::atomic_bool ThreadCallbackEnabled{false};

    virtual tjs_int FireLabelEventsAndGetNearestLabelEventStep(tjs_int64 tick) = 0;
    virtual void SetBufferPaused(bool bPaused) = 0;
    virtual bool FillBuffer(bool firstwrite, bool allowpause) = 0;
    virtual bool FillL2Buffer(bool firstwrite, bool fromdecodethread) = 0;
    virtual void FreeDirectSoundBuffer(bool disableevent) = 0;
    virtual void SetVolumeToSoundBuffer() = 0;
};

void TVPInitWaveDecodeThread();
void TVPUninitWaveDecodeThread();
void TVPAddDecodeSoundBuffer(iTVPDecodeSoundBuffer* buf);
void TVPRemoveDecodeSoundBuffer(iTVPDecodeSoundBuffer* buf);
void TVPWakeDecodeSoundBuffer();

extern bool TVPPrimarySoundBufferPlaying;
extern bool TVPDirectSoundShutdown;
extern bool TVPDeferedSettingAvailable;
void TVPEnsurePrimaryBufferPlay();
void TVPEnsureWaveSoundBufferWorking();
void TVPAddWaveSoundBuffer(iTVPDecodeSoundBuffer* buffer);
void TVPRemoveWaveSoundBuffer(iTVPDecodeSoundBuffer* buffer);
void TVPMakeSilentWave(void* dest, tjs_int count, const tTVPWaveFormat* format);
void TVPReschedulePendingLabelEvent(tjs_int tick);