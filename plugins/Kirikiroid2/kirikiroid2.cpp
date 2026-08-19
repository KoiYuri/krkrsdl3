#include "ncbind/ncbind.hpp"

#include <ctype.h>
#include "TextStream.h"
#include "TVPStorage.h"
#include "TVPSettings.h"
#include "TVPGraphicsLoader.h"

//---------------------------------------------------------------------------
// JPEG
//---------------------------------------------------------------------------
//0 = jlpLow
//1 = jlpMedium
//2 = jlpHigh
extern int TVPJPEGLoadPrecision;
//---------------------------------------------------------------------------

#define NCB_MODULE_NAME TJS_N("kirikiroid2.dll")

tjs_error krkr_str_ord(tTJSVariant* result,
                       tjs_int numparams,
                       tTJSVariant** param,
                       iTJSDispatch2* objthis)
{
    if (numparams == 0)
        return TJS_E_FAIL;
    else
    {
        tTJSVariant type = *param[0];
        if (type.Type() == tvtString)
        {
            const tTJSVariantString* vs = type.AsStringNoAddRef();
            const unsigned char* p = reinterpret_cast<const unsigned char*>(vs->ShortString);
            if (!*p)
                return 0;
            int tmp = 0;
            if ((*p & 0x80) == 0)
            {
                tmp = *p;
            }
            else if ((*p & 0xE0) == 0xC0)
            {
                tmp = ((*p & 0x1F) << 6) | (p[1] & 0x3F);
            }
            else if ((*p & 0xF0) == 0xE0)
            {
                tmp = ((*p & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
            }
            else if ((*p & 0xF8) == 0xF0)
            {
                tmp = ((*p & 0x07) << 18) | ((p[1] & 0x3F) << 12) | ((p[2] & 0x3F) << 6) |
                      (p[3] & 0x3F);
            }
            else
            {
                tmp = 0;
            }
            *result = (tjs_int)tmp;
        }
        else
        {
            *result = type;
        }
    }
    return TJS_S_OK;
}

NCB_REGISTER_FUNCTION(_str_ord, krkr_str_ord);

tjs_error krkr_setTextEncoding(tTJSVariant* result,
                               tjs_int numparams,
                               tTJSVariant** param,
                               iTJSDispatch2* objthis)
{
    if (numparams == 0)
        return TJS_E_FAIL;
    else
    {
        tTJSVariant type = *param[0];
        if (type.Type() == tvtString)
        {
            TVPSetDefaultReadEncoding(type);
        }
    }
    return TJS_S_OK;
}

NCB_ATTACH_FUNCTION(setTextEncoding, Storages, krkr_setTextEncoding);


extern tTVPArchive* TVPOpenTARArchive(const ttstr& name, tTJSBinaryStream* st, bool normalizeFileName);
extern tTVPArchive* TVPOpenZIPArchive(const ttstr& name, tTJSBinaryStream* st, bool normalizeFileName);
extern tTVPArchive* TVPOpen7ZArchive(const ttstr& name, tTJSBinaryStream* st, bool normalizeFileName);
extern tTVPRegisterGraphicInfo _jpegGraphicInfo;
extern tTVPRegisterGraphicInfo _pngGraphicInfo;
extern tTVPRegisterGraphicInfo _webpGraphicInfo;

void kirikiroid2_init()
{
    // 添加封包格式
    TVPRegisterArchiveFormat(TVPOpenTARArchive);
    TVPRegisterArchiveFormat(TVPOpenZIPArchive);
    TVPRegisterArchiveFormat(TVPOpen7ZArchive);
    // 添加图片格式
    tTJSVariant opt;
    // check TVPJPEGFastLoad option
    // this specifies precision for JPEG decoding
    if (TVPGetCommandLine(TJS_N("-jpegdec"), &opt))
    {
        ttstr str(opt);
        if (str == TJS_N("normal"))
            TVPJPEGLoadPrecision = 1;
        else if (str == TJS_N("low"))
            TVPJPEGLoadPrecision = 0;
        else if (str == TJS_N("high"))
            TVPJPEGLoadPrecision = 2;
    }
    TVPRegisterGraphicLoadingHandler(&_jpegGraphicInfo);
    TVPRegisterGraphicLoadingHandler(&_pngGraphicInfo);
    TVPRegisterGraphicLoadingHandler(&_webpGraphicInfo);
    // 添加音频格式
    
}

void kirikiroid2_done()
{
    TVPUnregisterArchiveFormat(TVPOpenTARArchive);
    TVPUnregisterArchiveFormat(TVPOpenZIPArchive);
    TVPUnregisterArchiveFormat(TVPOpen7ZArchive);
    TVPUnregisterGraphicLoadingHandler(&_jpegGraphicInfo);
    TVPUnregisterGraphicLoadingHandler(&_pngGraphicInfo);
    TVPUnregisterGraphicLoadingHandler(&_webpGraphicInfo);
}

NCB_PRE_REGIST_CALLBACK(kirikiroid2_init);
NCB_POST_UNREGIST_CALLBACK(kirikiroid2_done);