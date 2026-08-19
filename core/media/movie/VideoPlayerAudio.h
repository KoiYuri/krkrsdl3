#pragma once
#include "PlatformThread.h"
#include "IVideoPlayer.h"
#include "CodecAudio.h"
#include "MessageQueue.h"
#include "TVPTimer.h"
#include "VideoReferenceClock.h"
extern "C"
{
#include "libavutil/crc.h"
#include <libswresample/swresample.h>
#include "libavcodec/avcodec.h"
}
#include <cstdint>
#include "PlatformAudio.h"

NS_KRMOVIE_BEGIN

// AV sync options
enum AVSync
{
    SYNC_DISCON = 0,
    SYNC_RESAMPLE
};

class CVideoPlayerAudio : public tTVPThread, public IDVDStreamPlayerAudio
{
public:
    CVideoPlayerAudio(CDVDClock* pClock, CDVDMessageQueue& parent);
    virtual ~CVideoPlayerAudio();

    bool OpenStream(CDVDStreamInfo& hints) override;
    void CloseStream(bool bWaitForBuffers) override;

    void SetSpeed(int speed) override;
    void Flush(bool sync) override;

    // waits until all available data has been rendered
    bool AcceptsData() override;
    bool HasData() const override { return m_messageQueue.GetDataSize() > 0; }
    int GetLevel() override { return m_messageQueue.GetLevel(); }
    bool IsInited() const override { return m_messageQueue.IsInited(); }
    void SendMessage(CDVDMsg* pMsg, int priority = 0) override
    {
        m_messageQueue.Put(pMsg, priority);
    }
    void FlushMessages() override { m_messageQueue.Flush(); }
    float GetDynamicRangeAmplification() const override { return 0.0f; }

    std::string GetPlayerInfo() override;
    int GetAudioBitrate() override;
    int GetAudioChannels() override;

    // holds stream information for current playing stream
    CDVDStreamInfo m_streaminfo;

    double GetCurrentPts() override
    {
        tTJSUniqueLock lock(m_info_section);
        return m_info.pts;
    }

    bool IsStalled() const override { return m_stalled; }
    bool IsPassthrough() override;
    iTVPSoundBuffer* GetSoundBuffer() override { return m_soundDevice; }

protected:
    virtual void OnExit() override;
    virtual void Execute() override;

    void UpdatePlayerInfo();
    void OpenStream(CDVDStreamInfo& hints, CDVDAudioCodec* codec);
    //! Switch codec if needed. Called when the sample rate gotten from the
    //! codec changes, in which case we may want to switch passthrough on/off.
    bool SwitchCodecIfNeeded();

    CDVDMessageQueue m_messageQueue;
    CDVDMessageQueue& m_messageParent;

    double m_audioClock;

    CDVDClock* m_pClock;           // dvd master clock
    CDVDAudioCodec* m_pAudioCodec; // audio codec

    int m_speed;
    bool m_stalled;
    bool m_silence;
    bool m_paused;
    IDVDStreamPlayer::ESyncState m_syncState;
    TVPElapsedTimer m_syncTimer;

    bool ProcessDecoderOutput(DVDAudioFrame& audioframe);

    // SYNC_DISCON, SYNC_SKIPDUP, SYNC_RESAMPLE
    int m_synctype;
    int m_setsynctype;
    int m_prevsynctype; // so we can print to the log

    void SetSyncType(bool passthrough);

    bool m_prevskipped;
    double m_maxspeedadjust;

    struct SInfo
    {
        SInfo() : pts(DVD_NOPTS_VALUE), passthrough(false) {}

        std::string info;
        double pts;
        bool passthrough;
    };

    tTJSCriticalSection m_info_section;
    SInfo m_info;

    void Pause();
    bool Create(const DVDAudioFrame& audioframe, AVCodecID codec, bool needresampler);
    bool IsValidFormat(const DVDAudioFrame& audioframe);
    void Destroy();
    unsigned int AddPackets(const DVDAudioFrame& audioframe);
    double GetPlayingPts();
    double GetSyncError();
    void SetSyncErrorCorrection(double correction);
    void Flush();
    void AbortAddPackets();

    iTVPSoundBuffer* m_soundDevice = nullptr;
    struct SwrContext* swr_ctx = nullptr;
    AVSampleFormat swr_tgtFormat;
    unsigned int src_buffer_count = 0;
    unsigned int tgt_frameSize = 0;
    uint8_t* audio_buf = nullptr;
    unsigned int audio_buf_size = 0;
    double m_playingPts = DVD_NOPTS_VALUE;
    double m_timeOfPts = 0;
    double m_syncError = 0;
    unsigned int m_syncErrorTime = 0;
    std::atomic_bool m_bAbort;
    tTJSCriticalSection _mutex;
    tTVPCondition _cond;
    TVPElapsedTimer _timer;
};

NS_KRMOVIE_END
