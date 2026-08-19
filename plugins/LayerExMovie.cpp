#include <stdlib.h>
#include <stdint.h>
#include "tjsCommHead.h"
#include "TVPEvent.h"
#include "LayerExBase.hpp"
#include "ncbind/ncbind.hpp"
#include "TVPApplication.h"
#include "LayerBitmap.h"
#include <algorithm>
#include "krmovie.h"
#include "TVPStorage.h"
#include "PlatformThread.h"

#define NCB_MODULE_NAME TJS_N("layerExMovie.dll")

/*
 * Movie 描画用レイヤ
 */
struct layerExMovie : public layerExBase_GL, tTVPContinuousEventCallbackIntf, iTVPVideoCallback
{
protected:
    iTVPVideoOverlay* VideoOverlay;

    long movieWidth;
    long movieHeight;
    class tTVPBaseTexture* Bitmap[2];

    bool loop;
    bool alpha;

    tTJSBinaryStream* in;

    void clearMovie();

    DispatchT onStartMovie;
    DispatchT onUpdateMovie;
    DispatchT onStopMovie;

    bool playing;
    tTJSCriticalSection mtxEvent;
    std::vector<NativeEvent> PostEvents;

public:
    layerExMovie(DispatchT obj);
    ~layerExMovie();

public:
    // ムービーのロード
    void openMovie(const tjs_char* filename, bool alpha);

    void startMovie(bool loop);
    void stopMovie();

    void start();
    void stop();

    bool isPlayingMovie();

    void onUpdate();
    void onEnded();

    /**
     * Continuous コールバック
     * 吉里吉里が暇なときに常に呼ばれる
     * 塗り直し処理
     */
    virtual void OnContinuousCallback(tjs_uint64 tick) override;

    virtual void PostEvent(const NativeEvent& ev) override;
    virtual void WndProc(NativeEvent& ev) override;
};

/**
 * コンストラクタ
 */
layerExMovie::layerExMovie(DispatchT obj) : /*_pType(obj, TJS_N("type")),*/ layerExBase_GL(obj)
{
    VideoOverlay = NULL;
    loop = false;
    alpha = false;
    movieWidth = 0;
    movieHeight = 0;
    in = nullptr;
    {
        tTJSVariant var;
        if (TJS_SUCCEEDED(obj->PropGet(TJS_IGNOREPROP, TJS_N("onStartMovie"), NULL, &var, obj)))
            onStartMovie = var;
        else
            onStartMovie = NULL;
        if (TJS_SUCCEEDED(obj->PropGet(TJS_IGNOREPROP, TJS_N("onStopMovie"), NULL, &var, obj)))
            onStopMovie = var;
        else
            onStopMovie = NULL;
        if (TJS_SUCCEEDED(obj->PropGet(TJS_IGNOREPROP, TJS_N("onUpdateMovie"), NULL, &var, obj)))
            onUpdateMovie = var;
        else
            onUpdateMovie = NULL;
    }
    playing = false;
    // UtilWindow = new MessageDelegate(EVENT_FUNC2(layerExMovie, WndProc));
    Bitmap[0] = Bitmap[1] = nullptr;
}

/**
 * デストラクタ
 */
layerExMovie::~layerExMovie()
{
    stopMovie();
    // if (UtilWindow) delete UtilWindow;
    if (Bitmap[0])
        delete Bitmap[0];
    if (Bitmap[1])
        delete Bitmap[1];
}

void layerExMovie::clearMovie()
{
    if (VideoOverlay)
    {
        VideoOverlay->Release(), VideoOverlay = NULL;
    }
}

/**
 * ムービーファイルを開いて準備する
 * @param filename ファイル名
 * @param alpha アルファ指定（半分のサイズでα処理する）
 */
void layerExMovie::openMovie(const tjs_char* filename, bool alpha)
{
    clearMovie();
    this->alpha = alpha;
    movieWidth = 0;
    movieHeight = 0;

    // ファイルをテンポラリにコピーして対応
    if ((in = TVPCreateStream(filename, TJS_BS_READ)) == NULL)
    {
        ttstr error = filename;
        error += TJS_N(":ファイルが開けません");
        TVPAddLog(error);
        return;
    }
    ttstr ext = TVPExtractStorageExt(filename);
    ext.ToLowerCase();
    GetVideoLayerObject(this, in, filename, ext.c_str(), in->GetSize(), &VideoOverlay);
    VideoOverlay->GetVideoSize(&movieWidth, &movieHeight);
    if (Bitmap[0])
        delete Bitmap[0];
    if (Bitmap[1])
        delete Bitmap[1];
    long size = movieWidth * movieHeight * 4;
    Bitmap[0] = new tTVPBaseTexture(movieWidth, movieHeight /*, 32*/);
    Bitmap[1] = new tTVPBaseTexture(movieWidth, movieHeight /*, 32*/);
    VideoOverlay->SetVideoBuffer(Bitmap[0], Bitmap[1], size);
    if (alpha)
    {
        movieWidth /= 2;
    }
    _this->SetSize(movieWidth, movieHeight);
    _this->SetType(alpha ? ltAlpha : ltOpaque);
}

/**
 * ムービーの開始
 */
void layerExMovie::startMovie(bool loop)
{
    if (VideoOverlay)
    {
        // 再生開始
        this->loop = loop;
        VideoOverlay->Play();
        start();
        if (onStartMovie != NULL)
        {
            onStartMovie->FuncCall(0, NULL, NULL, NULL, 0, NULL, _obj);
        }
    }
}

/**
 * ムービーの停止
 */
void layerExMovie::stopMovie()
{
    bool p = playing;
    if (VideoOverlay)
        VideoOverlay->Stop();
    stop();
    clearMovie();
    if (p)
    {
        if (onStopMovie != NULL)
        {
            onStopMovie->FuncCall(0, NULL, NULL, NULL, 0, NULL, _obj);
        }
    }
}

bool layerExMovie::isPlayingMovie()
{
    return playing;
}

void layerExMovie::start()
{
    stop();
    TVPAddContinuousEventHook(this);
    playing = true;
}

/**
 * Irrlicht 呼び出し処理停止
 */
void layerExMovie::stop()
{
    TVPRemoveContinuousEventHook(this);
    playing = false;
}

void layerExMovie::onUpdate()
{
    // 更新完了
    // サーフェースからレイヤに画面コピー
    reset();
    tTVPBaseTexture* frontbmp = VideoOverlay->GetFrontBuffer();
    if (frontbmp)
    {
        iTVPTexture2D* src = frontbmp->GetTexture();
        iTVPTexture2D* dst = _this->GetMainImage()->GetTextureForRender(false, nullptr);
        tTVPRect rcdst(_clipLeft, _clipTop, _clipLeft + _clipWidth, _clipTop + _clipHeight);
        iTVPRenderMethod* method;
        if (alpha)
        {
            static iTVPRenderMethod* _method = TVPGetRenderManager()->GetRenderMethod("CopyColor");
            method = _method;
        }
        else
        {
            static iTVPRenderMethod* _method = TVPGetRenderManager()->GetRenderMethod("Copy");
            method = _method;
        }
        tRenderTexRectArray::Element src_tex[] = {
            tRenderTexRectArray::Element(src, tTVPRect(0, 0, std::min(_width, (tjs_int)movieWidth),
                                                       std::min(_height, (tjs_int)movieHeight)))};
        TVPGetRenderManager()->OperateRect(method, dst, nullptr, rcdst, src_tex);
    }
    // redraw();
    if (onUpdateMovie != NULL)
    {
        onUpdateMovie->FuncCall(0, NULL, NULL, NULL, 0, NULL, _obj);
    }
}

void layerExMovie::onEnded()
{
    // 更新終了
    if (loop)
    {
        VideoOverlay->Rewind();
        VideoOverlay->Play();
    }
    else
    {
        Application->PostUserMessage(std::bind(&layerExMovie::stopMovie, this));
    }
}

void layerExMovie::PostEvent(const NativeEvent& ev)
{
    tTJSCriticalSectionHolder lk(mtxEvent);
    PostEvents.push_back(ev);
}

void layerExMovie::WndProc(NativeEvent& ev)
{
    tTJSCriticalSectionHolder lk(mtxEvent);
    PostEvents.push_back(ev);
}

void layerExMovie::OnContinuousCallback(tjs_uint64 tick)
{
    if (VideoOverlay)
    {
        // 更新
        std::vector<NativeEvent> vecEvent;
        {
            tTJSCriticalSectionHolder lk(mtxEvent);
            vecEvent.swap(PostEvents);
        }

        for (NativeEvent msg : vecEvent)
        {
            switch (msg.Message)
            {
                case WM_GRAPHNOTIFY:
                {
                    switch (msg.WParam)
                    {
                        case EC_UPDATE:
                            onUpdate();
                            break;
                        case EC_COMPLETE:
                            onEnded();
                            break;
                        default:
                            break;
                    }
                    break;
                }
                default:
                    break;
            }
        }
    }
    else
    {
        stop();
    }
}

// ----------------------------------- クラスの登録

NCB_GET_INSTANCE_HOOK(layerExMovie){
    // インスタンスゲッタ
    NCB_INSTANCE_GETTER(objthis){
        // objthis を iTJSDispatch2* 型の引数とする
        ClassT* obj = GetNativeInstance(objthis); // ネイティブインスタンスポインタ取得
if (!obj)
{
    obj = new ClassT(objthis); // ない場合は生成する
    SetNativeInstance(objthis, obj); // objthis に obj をネイティブインスタンスとして登録する
}
return obj;
}

// デストラクタ（実際のメソッドが呼ばれた後に呼ばれる）
~NCB_GET_INSTANCE_HOOK_CLASS()
{
}
}
;

// フックつきアタッチ
NCB_ATTACH_CLASS_WITH_HOOK(layerExMovie, Layer)
{
    NCB_METHOD(openMovie);
    NCB_METHOD(startMovie);
    NCB_METHOD(stopMovie);
    NCB_METHOD(isPlayingMovie);
}
