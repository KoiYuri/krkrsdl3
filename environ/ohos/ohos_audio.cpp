// OHOS audio — OHAudio renderer with debug logging
// Pattern: SDL3's SDL_AudioStream — engine pushes, OHAudio pulls via callback.
// Ring buffer bridges push/pull. Buffer limit tracking per SDL3 pattern.

#include "tjsCommHead.h"
#include "PlatformAudio.h"
#include "PlatformMutex.h"
#include "TVPSystem.h"
#include "TVPDebug.h"

#include <ohaudio/native_audiorenderer.h>
#include <ohaudio/native_audiostreambuilder.h>
#include <hilog/log.h>
#include <cstring>
#include <atomic>
#include <pthread.h>

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "krkrsdl3_audio"

#define AUDIO_LOGI(fmt, ...) OH_LOG_INFO(LOG_APP, fmt, ##__VA_ARGS__)
#define AUDIO_LOGW(fmt, ...) OH_LOG_WARN(LOG_APP, fmt, ##__VA_ARGS__)

// ─── Ring buffer ────────────────────────────────────────────────
class RingBuf {
    uint8_t* _buf = nullptr;
    size_t _cap = 0;
    size_t _rpos = 0; // OHAudio reads
    size_t _wpos = 0; // AppendBuffer writes
    size_t _totalWritten = 0;
    size_t _totalRead = 0;
    pthread_mutex_t _mtx = PTHREAD_MUTEX_INITIALIZER;

public:
    ~RingBuf() { free(_buf); pthread_mutex_destroy(&_mtx); }
    void alloc(size_t cap) {
        _buf = (uint8_t*)malloc(cap);
        _cap = cap;
        AUDIO_LOGI("RingBuf alloc: %{public}zu bytes (%.1fms @44.1kHz/16bit/stereo)",
                   cap, cap * 1000.0 / (44100 * 4));
    }

    size_t capacity() const { return _cap; }

    // Non-blocking read from callback
    size_t readNB(uint8_t* dst, size_t want) {
        if (pthread_mutex_trylock(&_mtx) != 0) return 0;
        size_t avail = (_wpos >= _rpos) ? (_wpos - _rpos) : (_cap - _rpos + _wpos);
        size_t n = want < avail ? want : avail;
        if (n == 0) { pthread_mutex_unlock(&_mtx); return 0; }
        size_t tail = _cap - _rpos;
        if (n <= tail) {
            memcpy(dst, _buf + _rpos, n);
        } else {
            memcpy(dst, _buf + _rpos, tail);
            memcpy(dst + tail, _buf, n - tail);
        }
        _rpos = (_rpos + n) % _cap;
        _totalRead += n;
        pthread_mutex_unlock(&_mtx);
        return n;
    }

    // Blocking write — return false if not enough space
    bool write(const uint8_t* src, size_t len) {
        pthread_mutex_lock(&_mtx);
        size_t avail = (_wpos >= _rpos) ? (_wpos - _rpos) : (_cap - _rpos + _wpos);
        size_t free = _cap - avail - 1;
        if (len > free) { pthread_mutex_unlock(&_mtx); return false; }
        size_t tail = _cap - _wpos;
        if (len <= tail) {
            memcpy(_buf + _wpos, src, len);
        } else {
            memcpy(_buf + _wpos, src, tail);
            memcpy(_buf, src + tail, len - tail);
        }
        _wpos = (_wpos + len) % _cap;
        _totalWritten += len;
        pthread_mutex_unlock(&_mtx);
        return true;
    }

    size_t queued() { pthread_mutex_lock(&_mtx); size_t q = (_wpos>=_rpos)?(_wpos-_rpos):(_cap-_rpos+_wpos); pthread_mutex_unlock(&_mtx); return q; }
    void clear() { pthread_mutex_lock(&_mtx); _rpos = _wpos = 0; pthread_mutex_unlock(&_mtx); }
    void dumpStats() {
        AUDIO_LOGI("RingBuf: cap=%{public}zu written=%{public}zu read=%{public}zu queued=%{public}zu",
                   _cap, _totalWritten, _totalRead, queued());
    }
};

// ─── Sound buffer ──────────────────────────────────────────────
class tTVPSoundBufferOHOS : public iTVPSoundBuffer
{
    bool _playing = false;
    float _volume = 1, _pan = 0;
    int _sampleRate, _channels, _bps, _frameBytes;
    OH_AudioRenderer* _renderer = nullptr;
    RingBuf _ring;
    int _bufferLimitCount;
    int _appendCount = 0;    // 调试用
    int _cbCount = 0;        // 回调次数
    int _cbUnderrun = 0;     // 数据不足次数
    int _cbLockFail = 0;     // trylock 失败次数
    int _id;                 // 实例 ID
    static std::atomic<int> _nextId;

public:
    tTVPSoundBufferOHOS(tTVPWaveFormat& fmt, int bufcount)
      : _sampleRate(fmt.SamplesPerSec), _channels(fmt.Channels),
        _bps(fmt.BitsPerSample), _frameBytes(fmt.BitsPerSample * fmt.Channels / 8),
        _bufferLimitCount(bufcount), _id(_nextId.fetch_add(1))
    {
        AUDIO_LOGI("[%{public}d] CREATE: rate=%{public}d ch=%{public}d bps=%{public}d buflimit=%{public}d",
                   _id, _sampleRate, _channels, _bps, _bufferLimitCount);
    }

    virtual bool Init() override
    {
        // 2s buffer — WMV large frames (4k-6k samples) need more room
        size_t bufSize = _sampleRate * _frameBytes * 2;
        _ring.alloc(bufSize);

        OH_AudioStreamBuilder* builder = nullptr;
        OH_AudioStreamBuilder_Create(&builder, AUDIOSTREAM_TYPE_RENDERER);
        OH_AudioStreamBuilder_SetSamplingRate(builder, _sampleRate);
        OH_AudioStreamBuilder_SetChannelCount(builder, _channels);
        OH_AudioStream_SampleFormat fmt = (_bps == 8)  ? AUDIOSTREAM_SAMPLE_U8
                                        : (_bps == 32) ? AUDIOSTREAM_SAMPLE_S32LE
                                        : AUDIOSTREAM_SAMPLE_S16LE;
        OH_AudioStreamBuilder_SetSampleFormat(builder, fmt);
        OH_AudioStreamBuilder_SetEncodingType(builder, AUDIOSTREAM_ENCODING_TYPE_RAW);
        OH_AudioStreamBuilder_SetLatencyMode(builder, AUDIOSTREAM_LATENCY_MODE_FAST);
        OH_AudioStreamBuilder_SetRendererInfo(builder, AUDIOSTREAM_USAGE_MUSIC);
        OH_AudioStreamBuilder_SetRendererWriteDataCallback(builder, OnWriteData, this);
        OH_AudioStreamBuilder_GenerateRenderer(builder, &_renderer);
        OH_AudioStreamBuilder_Destroy(builder);

        AUDIO_LOGI("[%{public}d] Init: renderer=%{public}p fmt=%{public}d bufSize=%{public}zu",
                   _id, _renderer, (int)fmt, bufSize);
        return _renderer != nullptr;
    }

    static OH_AudioData_Callback_Result OnWriteData(OH_AudioRenderer*, void* userData, void* buffer, int32_t length)
    {
        auto* self = static_cast<tTVPSoundBufferOHOS*>(userData);
        self->_cbCount++;

        size_t got = self->_ring.readNB((uint8_t*)buffer, length);
        if (got == 0) {
            // 无数据：填静音。绝不返回 INVALID（会导致系统重复回调，浪费 CPU）
            memset(buffer, 0, length);
            self->_cbLockFail++;
        } else if (got < (size_t)length) {
            memset((uint8_t*)buffer + got, 0, length - got);
            self->_cbUnderrun++;
        }

        if (self->_cbCount % 500 == 0) {
            AUDIO_LOGI("[%{public}d] cb#%{public}d: want=%{public}d got=%{public}zu under=%{public}d lock=%{public}d append=%{public}d",
                       self->_id, self->_cbCount, length, got, self->_cbUnderrun, self->_cbLockFail, self->_appendCount);
        }
        return AUDIO_DATA_CALLBACK_RESULT_VALID;
    }

    virtual ~tTVPSoundBufferOHOS() {
        AUDIO_LOGI("[%{public}d] DESTROY: append=%{public}d cb=%{public}d under=%{public}d lock=%{public}d",
                   _id, _appendCount, _cbCount, _cbUnderrun, _cbLockFail);
        _ring.dumpStats();
        Stop();
        if (_renderer) OH_AudioRenderer_Release(_renderer);
    }

    virtual void Release() override { delete this; }

    virtual void Play() override {
        if (_playing || !_renderer) return;
        OH_AudioRenderer_Start(_renderer);
        _playing = true;
        AUDIO_LOGI("[%{public}d] Play", _id);
    }
    virtual void Pause() override {
        if (!_playing || !_renderer) return;
        OH_AudioRenderer_Pause(_renderer);
        _playing = false;
        AUDIO_LOGI("[%{public}d] Pause", _id);
    }
    virtual void Stop() override {
        AUDIO_LOGI("[%{public}d] Stop", _id);
        _playing = false;
        if (_renderer) { OH_AudioRenderer_Stop(_renderer); OH_AudioRenderer_Flush(_renderer); }
        _ring.clear();
    }
    virtual void Reset() override {
        _playing = false;
        if (_renderer) { OH_AudioRenderer_Stop(_renderer); OH_AudioRenderer_Flush(_renderer); }
        _ring.clear(); // Reset 才清空（创建新 buffer 时调用）
    }
    virtual bool IsPlaying() override { return _playing; }
    virtual void SetVolume(float v) override {
        _volume = v;
        if (_renderer) OH_AudioRenderer_SetVolume(_renderer, v);
    }
    virtual float GetVolume() override { return _volume; }
    virtual void SetPan(float v) override { _pan = v; }
    virtual float GetPan() override { return _pan; }

    virtual void AppendBuffer(const void* data, unsigned int len) override {
        if (len == 0) return;
        _ring.write((const uint8_t*)data, len);
        _appendCount++;
    }

    virtual bool IsBufferValid() override {
        // SDL3 pattern: limit queued buffer count
        size_t q = _ring.queued();
        // Each "buffer" is roughly _frameBytes * typical samples
        size_t bufSize = _frameBytes * (_sampleRate / 20); // ~50ms per buffer
        return (q / bufSize) < (size_t)_bufferLimitCount;
    }
    virtual bool IsValidFormat(tTVPWaveFormat& fmt) override {
        return _sampleRate == fmt.SamplesPerSec && _bps == fmt.BitsPerSample && _channels == fmt.Channels;
    }
    virtual tjs_uint GetCurrentPlaySamples() override { return (tjs_uint)(_ring.queued() / _frameBytes); }
    virtual int GetRemainBuffers() override {
        size_t q = _ring.queued();
        size_t bufSize = _frameBytes * (_sampleRate / 20);
        return (int)(q / bufSize);
    }
    virtual tjs_uint GetLatencySamples() override { return 0; }
    virtual float GetLatencySeconds() override { return 0; }
    virtual void SetPosition(float, float, float) override {}
};

std::atomic<int> tTVPSoundBufferOHOS::_nextId{0};

// ─── Factory ───────────────────────────────────────────────────
void TVPInitDirectSound(int freq)
{
    AUDIO_LOGI("TVPInitDirectSound freq=%{public}d", freq);
}

void TVPUninitDirectSound()
{
    AUDIO_LOGI("TVPUninitDirectSound");
}

iTVPSoundBuffer* TVPCreateSoundBuffer(tTVPWaveFormat& fmt, int bufcount)
{
    AUDIO_LOGI("TVPCreateSoundBuffer: rate=%{public}d ch=%{public}d bps=%{public}d count=%{public}d",
               fmt.SamplesPerSec, fmt.Channels, fmt.BitsPerSample, bufcount);
    auto* s = new tTVPSoundBufferOHOS(fmt, bufcount);
    if (s->Init()) return s;
    delete s;
    return nullptr;
}
