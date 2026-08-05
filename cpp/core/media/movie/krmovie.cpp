#include "tjsCommHead.h"

#include "krmovie.h"
#include "TVPMsg.h"
#include "TVPStorage.h"
#include "TVPEvent.h"
#include "Platform.h"
#include "PlatformVideo.h"
#include "krnull.h"

#ifdef _KRKRSDL3_USE_FFMPEG
#include "KRMovieOverlay.h"
#include "KRMovieLayer.h"
#endif

// ─── System (non-FFmpeg) video player factory ───────────────
// Always succeeds — returns a working player or a null/stub player
// so the caller never hits a null pointer.

static void VideoPostEvent(void* ctx, unsigned msg, uintptr_t wparam, uintptr_t lparam)
{
    auto* cb = static_cast<iTVPVideoCallback*>(ctx);
    NativeEvent ev(msg);
    ev.WParam = wparam;
    ev.LParam = lparam;
    if (wparam == EC_COMPLETE)
        cb->PostEvent(ev);
    else
        cb->WndProc(ev);
}

static void TryCreateOverlay(iTVPVideoCallback* callbackwin,
                              tTJSBinaryStream* stream,
                              const tjs_char* streamname,
                              const tjs_char* /*type*/,
                              uint64_t /*size*/,
                              iTVPVideoOverlay** out)
{
    OverlayVideoPlayer* player = CreateOverlayVideoPlayer(VideoPostEvent, callbackwin);
    if (player && player->OpenStream(stream, ttstr(streamname))) {
        *out = player;
        return;
    }
    if (player)
        delete player;
    *out = new NullOverlayPlayer(callbackwin);
}

static void TryCreateLayer(iTVPVideoCallback* callbackwin,
                            tTJSBinaryStream* stream,
                            const tjs_char* streamname,
                            const tjs_char* /*type*/,
                            uint64_t /*size*/,
                            iTVPVideoOverlay** out)
{
    LayerVideoPlayer* player = CreateLayerVideoPlayer(VideoPostEvent, callbackwin);
    if (player && player->OpenStream(stream, ttstr(streamname))) {
        *out = player;
        return;
    }
    if (player)
        delete player;
    *out = new NullLayerPlayer(callbackwin);
}

void GetVideoOverlayObject(iTVPVideoCallback* callbackwin,
                           tTJSBinaryStream* stream,
                           const tjs_char* streamname,
                           const tjs_char* type,
                           uint64_t size,
                           iTVPVideoOverlay** out)
{
#ifdef _KRKRSDL3_USE_FFMPEG
    *out = new KRMovie::MoviePlayerOverlay;
    if (*out)
        static_cast<KRMovie::MoviePlayerOverlay*>(*out)->BuildGraph(callbackwin, stream, streamname,
                                                                    type, size);
#else
    TryCreateOverlay(callbackwin, stream, streamname, type, size, out);
#endif
}

void GetVideoLayerObject(iTVPVideoCallback* callbackwin,
                         tTJSBinaryStream* stream,
                         const tjs_char* streamname,
                         const tjs_char* type,
                         uint64_t size,
                         iTVPVideoOverlay** out)
{
#ifdef _KRKRSDL3_USE_FFMPEG
    *out = new KRMovie::MoviePlayerLayer;
    if (*out)
        static_cast<KRMovie::MoviePlayerLayer*>(*out)->BuildGraph(callbackwin, stream, streamname,
                                                                  type, size);
#else
    TryCreateLayer(callbackwin, stream, streamname, type, size, out);
#endif
}

void GetMixingVideoOverlayObject(iTVPVideoCallback* callbackwin,
                                 tTJSBinaryStream* stream,
                                 const tjs_char* streamname,
                                 const tjs_char* type,
                                 uint64_t size,
                                 iTVPVideoOverlay** out)
{
#ifdef _KRKRSDL3_USE_FFMPEG
    *out = new KRMovie::MoviePlayerOverlay;
    if (*out)
        static_cast<KRMovie::MoviePlayerOverlay*>(*out)->BuildGraph(callbackwin, stream, streamname,
                                                                    type, size);
#else
    TryCreateOverlay(callbackwin, stream, streamname, type, size, out);
#endif
}

void GetMFVideoOverlayObject(iTVPVideoCallback* callbackwin,
                             tTJSBinaryStream* stream,
                             const tjs_char* streamname,
                             const tjs_char* type,
                             uint64_t size,
                             iTVPVideoOverlay** out)
{
#ifdef _KRKRSDL3_USE_FFMPEG
    *out = new KRMovie::MoviePlayerOverlay;
    if (*out)
        static_cast<KRMovie::MoviePlayerOverlay*>(*out)->BuildGraph(callbackwin, stream, streamname,
                                                                    type, size);
#else
    TryCreateOverlay(callbackwin, stream, streamname, type, size, out);
#endif
}