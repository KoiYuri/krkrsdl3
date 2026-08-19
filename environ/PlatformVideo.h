#pragma once

#include "krmovie.h"
#include "PlatformAudio.h"
#include "PlatformThread.h"

namespace TJS { class tTJSBinaryStream; }

// ─── Overlay: self-managed display ─────────────────────────
// Renders decoded video directly onto the screen (via sprite/texture).
// Used by tjsNativeVideoOverlay in vomOverlay / vomMixer mode.
class OverlayVideoPlayer : public iTVPVideoOverlay
{
public:
    virtual ~OverlayVideoPlayer() = default;

    // Open a video stream. The implementation saves it to a temporary file
    // if the underlying platform API requires a file path.
    virtual bool OpenStream(TJS::tTJSBinaryStream* stream, const ttstr& streamname) = 0;

    // Periodic callback from the engine — present the latest frame.
    virtual void OnContinuousCallback(tjs_uint64 tick) = 0;
};

// ─── Layer: pixel output ───────────────────────────────────
// Decodes video and outputs RGBA pixels via GetFrontBuffer().
// The layer system calls GetFrontBuffer() each frame.
// Used by LayerExMovie and vomLayer mode.
class LayerVideoPlayer : public iTVPVideoOverlay
{
public:
    virtual ~LayerVideoPlayer() = default;

    virtual bool OpenStream(TJS::tTJSBinaryStream* stream, const ttstr& streamname) = 0;
    virtual void OnContinuousCallback(tjs_uint64 tick) = 0;
    virtual void SetVideoBuffer(tTVPBaseTexture* buff1, tTVPBaseTexture* buff2, long size) = 0;
    virtual tTVPBaseTexture* GetFrontBuffer() = 0;
};

// ─── Factory ───────────────────────────────────────────────
// Returns nullptr if platform backend is unavailable (caller falls back to FFmpeg).
//
// Event callback: (ctx, msg, wparam, lparam)
//   msg == 0x8000 (WM_GRAPHNOTIFY), wparam == EC_COMPLETE/EC_UPDATE
// Implementations should post EC_COMPLETE through event queue,
// and dispatch EC_UPDATE synchronously via WndProc.
typedef void (*TVPVideoEventCallback)(void* ctx, unsigned msg, uintptr_t wparam, uintptr_t lparam);
OverlayVideoPlayer* CreateOverlayVideoPlayer(TVPVideoEventCallback cb, void* cbctx);
LayerVideoPlayer*    CreateLayerVideoPlayer(TVPVideoEventCallback cb, void* cbctx);