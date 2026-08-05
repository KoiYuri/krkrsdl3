#include "tjsCommHead.h"

#include "TVPGraphicsLoader.h"
#include "LayerBitmap.h"
#include "TVPStorage.h"
#include "TVPMsg.h"
#include "TVPScript.h"

#include "RenderManager.h"

#include "tjsDictionary.h"

//---------------------------------------------------------------------------
// BMP loading interface
//---------------------------------------------------------------------------

#ifndef BI_RGB // avoid re-define error on Win32
#define BI_RGB 0
#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3
#endif

#pragma pack(push, 1)
struct TVP_WIN_BITMAPFILEHEADER
{
    tjs_uint16 bfType;
    tjs_uint32 bfSize;
    tjs_uint16 bfReserved1;
    tjs_uint16 bfReserved2;
    tjs_uint32 bfOffBits;
};
struct TVP_WIN_BITMAPINFOHEADER
{
    tjs_uint32 biSize;
    tjs_int biWidth;
    tjs_int biHeight;
    tjs_uint16 biPlanes;
    tjs_uint16 biBitCount;
    tjs_uint32 biCompression;
    tjs_uint32 biSizeImage;
    tjs_int biXPelsPerMeter;
    tjs_int biYPelsPerMeter;
    tjs_uint32 biClrUsed;
    tjs_uint32 biClrImportant;
};
#pragma pack(pop)

enum tTVPBMPAlphaType
{
    // this specifies alpha channel treatment if the bitmap is 32bpp.
    // note that TVP currently does not support new (V4 or V5) bitmap header
    batNone,     // plugin does not return alpha channel.
    batMulAlpha, // returns alpha channel, d = d * alpha + s * (1-alpha)
    batAddAlpha  // returns alpha channel, d = d * alpha + s
};


//---------------------------------------------------------------------------
// BMP loading handler
//---------------------------------------------------------------------------
bool TVPQuickTestBMP(tTJSBinaryStream* src)
{
    uint8_t header[2];
    tjs_uint64 origSrcPos = src->GetPosition();
    if (src->Read(header, sizeof(header)) == sizeof(header))
    {
        src->SetPosition(origSrcPos);
    }
    if (!memcmp(header, "BM", 2))
    {
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
bool TVPAcceptSaveAsBMP(void* formatdata, const ttstr& type, class iTJSDispatch2** dic)
{
    bool result = false;
    if (type.StartsWith(TJS_N("bmp")))
        result = true;
    else if (type == TJS_N(".bmp"))
        result = true;
    else if (type == TJS_N(".dib"))
        result = true;
    if (result && dic)
    {
        tTJSVariant result;
        TVPExecuteExpression(TJS_N("(const)%[")
                                 TJS_N("\"bpp\"=>(const)%[\"type\"=>\"select\",\"items\"=>(const)["
                                       "\"32\",\"24\",\"8\"],\"desc\"=>\"bpp\",\"default\"=>0]")
                                     TJS_N("]"),
                             NULL, &result);
        if (result.Type() == tvtObject)
        {
            *dic = result.AsObject();
        }
        //*dic = TJSCreateDictionaryObject();
    }
    return result;
}

//---------------------------------------------------------------------------
#define TVP_BMP_READ_LINE_MAX 8
static void TVPInternalLoadBMP(void* callbackdata,
                        tTVPGraphicSizeCallback sizecallback,
                        tTVPGraphicScanLineCallback scanlinecallback,
                        TVP_WIN_BITMAPINFOHEADER& bi,
                        const tjs_uint8* palsrc,
                        tTJSBinaryStream* src,
                        tjs_int keyidx,
                        tTVPBMPAlphaType alphatype,
                        tTVPGraphicLoadMode mode)
{
    // mostly taken ( but totally re-written ) from SDL,
    // http://www.libsdl.org/

    // TODO: only checked on Win32 platform

    if (bi.biSize == 12)
    {
        // OS/2
        bi.biCompression = BI_RGB;
        bi.biClrUsed = 1 << bi.biBitCount;
    }

    tjs_uint16 orgbitcount = bi.biBitCount;
    if (bi.biBitCount == 1 || bi.biBitCount == 4)
    {
        bi.biBitCount = 8;
    }

    switch (bi.biCompression)
    {
        case BI_RGB:
            // if there are no masks, use the defaults
            break; // use default
                   /*
                                   if( bf.bfOffBits == ( 14 + bi.biSize) )
                                   {
                                   }
                                   // fall through -- read the RGB masks
                   */
        case BI_BITFIELDS:
            TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPBitFieldsNotSupported);

        default:
            TVPThrowExceptionMessage(TVPImageLoadError,
                                     (const tjs_char*)TVPCompressedBmpNotSupported);
    }

    // load palette
    tjs_uint32 palette[256]; // (msb) argb (lsb)
    if (orgbitcount <= 8)
    {
        if (bi.biClrUsed == 0)
            bi.biClrUsed = 1 << orgbitcount;
        if (bi.biSize == 12)
        {
            // read OS/2 palette
            for (tjs_uint i = 0; i < bi.biClrUsed; i++)
            {
                palette[i] = (palsrc[0] << 16) + (palsrc[1] << 8) + (palsrc[2] << 0) + 0xff000000;
                palsrc += 3;
            }
        }
        else
        {
            // read Windows palette
            for (tjs_uint i = 0; i < bi.biClrUsed; i++)
            {
                palette[i] = (palsrc[0] << 16) + (palsrc[1] << 8) + (palsrc[2] << 0) + 0xff000000;
                // we assume here that the palette's unused segment is useless.
                // fill it with 0xff ( = completely opaque )
                palsrc += 4;
            }
        }

        if (mode == glmGrayscale)
        {
            TVPDoGrayScale(palette, 256);
        }

        if (keyidx != -1)
        {
            // if color key by palette index is specified
            palette[keyidx & 0xff] &= 0x00ffffff; // make keyidx transparent
        }
    }
    else
    {
        if (mode == glmPalettized)
            TVPThrowExceptionMessage(TVPImageLoadError,
                                     (const tjs_char*)TVPUnsupportedColorModeForPalettImage);
    }

    tjs_int height;
    height = bi.biHeight < 0 ? -bi.biHeight : bi.biHeight;
    // positive value of bi.biHeight indicates top-down DIB

    tTVPGraphicPixelFormat pixfmt;
    switch (orgbitcount)
    {
        case 32:
            pixfmt = gpfRGBA;
            break;
        case 24:
        case 16:
        case 15:
            pixfmt = gpfRGB;
            break;
        default:
            pixfmt = gpfPalette;
            break;
    }
    sizecallback(callbackdata, bi.biWidth, height, pixfmt);

    tjs_int pitch;
    pitch = (((bi.biWidth * orgbitcount) + 31) & ~31) / 8;
    tjs_uint8* readbuf = (tjs_uint8*)TJSAlignedAlloc(pitch * TVP_BMP_READ_LINE_MAX, 4);
    tjs_uint8* buf;
    tjs_int bufremain = 0;
    try
    {
        // process per a line
        tjs_int src_y = 0;
        tjs_int dest_y;
        if (bi.biHeight > 0)
            dest_y = bi.biHeight - 1;
        else
            dest_y = 0;

        for (; src_y < height; src_y++)
        {
            if (bufremain == 0)
            {
                tjs_int remain = height - src_y;
                tjs_int read_lines =
                    remain > TVP_BMP_READ_LINE_MAX ? TVP_BMP_READ_LINE_MAX : remain;
                src->ReadBuffer(readbuf, pitch * read_lines);
                bufremain = read_lines;
                buf = readbuf;
            }

            void* scanline = scanlinecallback(callbackdata, dest_y);
            if (!scanline)
                break;

            switch (orgbitcount)
            {
                    // convert pixel format
                case 1:
                    if (mode == glmPalettized)
                    {
                        TVPBLExpand1BitTo8Bit((tjs_uint8*)scanline, (tjs_uint8*)buf, bi.biWidth);
                    }
                    else if (mode == glmGrayscale)
                    {
                        TVPBLExpand1BitTo8BitPal((tjs_uint8*)scanline, (tjs_uint8*)buf, bi.biWidth,
                                                 palette);
                    }
                    else
                    {
                        TVPBLExpand1BitTo32BitPal((tjs_uint32*)scanline, (tjs_uint8*)buf,
                                                  bi.biWidth, palette);
                    }
                    break;

                case 4:
                    if (mode == glmPalettized)
                    {
                        TVPBLExpand4BitTo8Bit((tjs_uint8*)scanline, (tjs_uint8*)buf, bi.biWidth);
                    }
                    else if (mode == glmGrayscale)
                    {
                        TVPBLExpand4BitTo8BitPal((tjs_uint8*)scanline, (tjs_uint8*)buf, bi.biWidth,
                                                 palette);
                    }
                    else
                    {
                        TVPBLExpand4BitTo32BitPal((tjs_uint32*)scanline, (tjs_uint8*)buf,
                                                  bi.biWidth, palette);
                    }
                    break;

                case 8:
                    if (mode == glmPalettized)
                    {
                        // intact copy
                        memcpy(scanline, buf, bi.biWidth);
                    }
                    else if (mode == glmGrayscale)
                    {
                        // convert to grayscale
                        TVPBLExpand8BitTo8BitPal((tjs_uint8*)scanline, (tjs_uint8*)buf, bi.biWidth,
                                                 palette);
                    }
                    else
                    {
                        TVPBLExpand8BitTo32BitPal((tjs_uint32*)scanline, (tjs_uint8*)buf,
                                                  bi.biWidth, palette);
                    }
                    break;

                case 15:
                case 16:
                    if (mode == glmGrayscale)
                    {
                        TVPBLConvert15BitTo8Bit((tjs_uint8*)scanline, (tjs_uint16*)buf, bi.biWidth);
                    }
                    else
                    {
                        TVPBLConvert15BitTo32Bit((tjs_uint32*)scanline, (tjs_uint16*)buf,
                                                 bi.biWidth);
                    }
                    break;

                case 24:
                    if (mode == glmGrayscale)
                    {
                        TVPBLConvert24BitTo8Bit((tjs_uint8*)scanline, (tjs_uint8*)buf, bi.biWidth);
                    }
                    else
                    {
                        TVPBLConvert24BitTo32Bit((tjs_uint32*)scanline, (tjs_uint8*)buf,
                                                 bi.biWidth);
                    }
                    break;

                case 32:
                    if (mode == glmGrayscale)
                    {
                        TVPBLConvert32BitTo8Bit((tjs_uint8*)scanline, (tjs_uint32*)buf, bi.biWidth);
                    }
                    else
                    {
                        if (alphatype == batNone)
                        {
                            // alpha channel is not given by the bitmap.
                            // destination alpha is filled with 255.
                            TVPBLConvert32BitTo32Bit_NoneAlpha((tjs_uint32*)scanline,
                                                               (tjs_uint32*)buf, bi.biWidth);
                        }
                        else if (alphatype == batMulAlpha)
                        {
                            // this is the TVP native representation of the alpha channel.
                            // simply copy from the buffer.
                            TVPBLConvert32BitTo32Bit_MulAddAlpha((tjs_uint32*)scanline,
                                                                 (tjs_uint32*)buf, bi.biWidth);
                        }
                        else if (alphatype == batAddAlpha)
                        {
                            // this is alternate representation of the alpha channel,
                            // this must be converted to TVP native representation.
                            TVPBLConvert32BitTo32Bit_AddAlpha((tjs_uint32*)scanline,
                                                              (tjs_uint32*)buf, bi.biWidth);
                        }
                    }
                    break;
            }

            scanlinecallback(callbackdata, -1); // image was written

            if (bi.biHeight > 0)
                dest_y--;
            else
                dest_y++;
            buf += pitch;
            bufremain--;
        }
    }
    catch (...)
    {
        TJSAlignedDealloc(readbuf);
        throw;
    }

    TJSAlignedDealloc(readbuf);
}
//---------------------------------------------------------------------------
void TVPLoadBMP(void* formatdata,
                void* callbackdata,
                tTVPGraphicSizeCallback sizecallback,
                tTVPGraphicScanLineCallback scanlinecallback,
                tTVPMetaInfoPushCallback metainfopushcallback,
                tTJSBinaryStream* src,
                tjs_int keyidx,
                tTVPGraphicLoadMode mode)
{
    // Windows BMP Loader
    // mostly taken ( but totally re-written ) from SDL,
    // http://www.libsdl.org/

    // TODO: only checked in Win32 platform

    tjs_uint64 firstpos = src->GetPosition();

    // check the magic
    tjs_uint8 magic[2];
    src->ReadBuffer(magic, 2);
    if (magic[0] != 'B' || magic[1] != 'M')
        TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPNotWindowsBmp);

    // read the BITMAPFILEHEADER
    TVP_WIN_BITMAPFILEHEADER bf;
    bf.bfSize = src->ReadI32LE();
    bf.bfReserved1 = src->ReadI16LE();
    bf.bfReserved2 = src->ReadI16LE();
    bf.bfOffBits = src->ReadI32LE();

    // read the BITMAPINFOHEADER
    TVP_WIN_BITMAPINFOHEADER bi;
    bi.biSize = src->ReadI32LE();
    if (bi.biSize == 12)
    {
        // OS/2 Bitmap
        memset(&bi, 0, sizeof(bi));
        bi.biWidth = (tjs_uint32)src->ReadI16LE();
        bi.biHeight = (tjs_uint32)src->ReadI16LE();
        bi.biPlanes = src->ReadI16LE();
        bi.biBitCount = src->ReadI16LE();
        bi.biClrUsed = 1 << bi.biBitCount;
    }
    else if (bi.biSize == 40)
    {
        // Windows Bitmap
        bi.biWidth = src->ReadI32LE();
        bi.biHeight = src->ReadI32LE();
        bi.biPlanes = src->ReadI16LE();
        bi.biBitCount = src->ReadI16LE();
        bi.biCompression = src->ReadI32LE();
        bi.biSizeImage = src->ReadI32LE();
        bi.biXPelsPerMeter = src->ReadI32LE();
        bi.biYPelsPerMeter = src->ReadI32LE();
        bi.biClrUsed = src->ReadI32LE();
        bi.biClrImportant = src->ReadI32LE();
    }
    else
    {
        TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPUnsupportedHeaderVersion);
    }

    // load palette
    tjs_int palsize = (bi.biBitCount <= 8)
                          ? ((bi.biClrUsed == 0 ? (1 << bi.biBitCount) : bi.biClrUsed) *
                             ((bi.biSize == 12) ? 3 : 4))
                          : 0; // bi.biSize == 12 ( OS/2 palette )
    tjs_uint8* palette = NULL;

    if (palsize)
        palette = new tjs_uint8[palsize];

    try
    {
        src->ReadBuffer(palette, palsize);
        src->SetPosition(firstpos + bf.bfOffBits);

        TVPInternalLoadBMP(callbackdata, sizecallback, scanlinecallback, bi, palette, src, keyidx,
                           batMulAlpha, mode);
    }
    catch (...)
    {
        if (palette)
            delete[] palette;
        throw;
    }
    if (palette)
        delete[] palette;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// BMP saving handler
//---------------------------------------------------------------------------
static void TVPWriteLE16(tTJSBinaryStream* stream, tjs_uint16 number)
{
    tjs_uint8 data[2];
    data[0] = number & 0xff;
    data[1] = (number >> 8) & 0xff;
    stream->WriteBuffer(data, 2);
}
//---------------------------------------------------------------------------
static void TVPWriteLE32(tTJSBinaryStream* stream, tjs_uint32 number)
{
    tjs_uint8 data[4];
    data[0] = number & 0xff;
    data[1] = (number >> 8) & 0xff;
    data[2] = (number >> 16) & 0xff;
    data[3] = (number >> 24) & 0xff;
    stream->WriteBuffer(data, 4);
}
//---------------------------------------------------------------------------
void TVPSaveTextureAsBMP(tTJSBinaryStream* dst,
                         iTVPTexture2D* bmp,
                         const ttstr& mode,
                         iTJSDispatch2* meta)
{
    tjs_int pixelbytes;

    if (bmp->GetFormat() == TVPTextureFormat::Gray)
        pixelbytes = 1;
    else if (mode == TJS_N("bmp32") || mode == TJS_N("bmp"))
        pixelbytes = 4;
    else if (mode == TJS_N("bmp24"))
        pixelbytes = 3;
    else if (mode == TJS_N("bmp8"))
        pixelbytes = 1;
    else
        pixelbytes = 4;

    if (meta)
    {
        tTJSVariant val;
        tjs_error er = meta->PropGet(TJS_MEMBERMUSTEXIST, TJS_N("bpp"), NULL, &val, meta);
        if (TJS_SUCCEEDED(er))
        {
            tjs_int index = (tjs_int)val.AsInteger();
            switch (index)
            {
                case 0:
                    pixelbytes = 4;
                    break;
                case 1:
                    pixelbytes = 3;
                    break;
                case 2:
                    pixelbytes = 1;
                    break;
            };
        }
    }

    // open stream
    tTJSBinaryStream* stream = dst;
    tjs_uint8* buf = NULL;

    try
    {
        TVPClearGraphicCache();

        // prepare header
        tjs_uint bmppitch = bmp->GetWidth() * pixelbytes;
        bmppitch = (((bmppitch - 1) >> 2) + 1) << 2;

        TVPWriteLE16(stream, 0x4d42); /* bfType */
        TVPWriteLE32(stream, sizeof(TVP_WIN_BITMAPFILEHEADER) + sizeof(TVP_WIN_BITMAPINFOHEADER) +
                                 bmppitch * bmp->GetHeight() +
                                 (pixelbytes == 1 ? 1024 : 0)); /* bfSize */
        TVPWriteLE16(stream, 0);                                /* bfReserved1 */
        TVPWriteLE16(stream, 0);                                /* bfReserved2 */
        TVPWriteLE32(stream, sizeof(TVP_WIN_BITMAPFILEHEADER) + sizeof(TVP_WIN_BITMAPINFOHEADER) +
                                 (pixelbytes == 1 ? 1024 : 0)); /* bfOffBits */

        TVPWriteLE32(stream, sizeof(TVP_WIN_BITMAPINFOHEADER)); /* biSize */
        TVPWriteLE32(stream, bmp->GetWidth());                  /* biWidth */
        TVPWriteLE32(stream, bmp->GetHeight());                 /* biHeight */
        TVPWriteLE16(stream, 1);                                /* biPlanes */
        TVPWriteLE16(stream, pixelbytes * 8);                   /* biBitCount */
        TVPWriteLE32(stream, BI_RGB);                           /* biCompression */
        TVPWriteLE32(stream, 0);                                /* biSizeImage */
        TVPWriteLE32(stream, 0);                                /* biXPelsPerMeter */
        TVPWriteLE32(stream, 0);                                /* biYPelsPerMeter */
        TVPWriteLE32(stream, 0);                                /* biClrUsed */
        TVPWriteLE32(stream, 0);                                /* biClrImportant */

        // write palette
        if (pixelbytes == 1)
        {
            tjs_uint8 palette[1024];
            tjs_uint8* p = palette;
            if (bmp->GetFormat() == TVPTextureFormat::Gray)
            {
                for (tjs_int i = 0; i < 256; i++)
                {
                    p[0] = i;
                    p[1] = i;
                    p[2] = i;
                    p[3] = 0;
                    p += 4;
                }
            }
            else
            {
                for (tjs_int i = 0; i < 256; i++)
                {
                    p[0] = TVP252DitherPalette[0][i];
                    p[1] = TVP252DitherPalette[1][i];
                    p[2] = TVP252DitherPalette[2][i];
                    p[3] = 0;
                    p += 4;
                }
            }
            stream->WriteBuffer(palette, 1024);
        }

        // write bitmap body
        if (bmp->GetFormat() == TVPTextureFormat::Gray)
        {
            for (tjs_int y = bmp->GetHeight() - 1; y >= 0; y--)
            {
                if (!buf)
                    buf = new tjs_uint8[bmppitch];
                memcpy(buf, bmp->GetScanLineForRead(y), bmp->GetWidth());
                stream->WriteBuffer(buf, bmppitch);
            }
        }
        else
        {
            for (tjs_int y = bmp->GetHeight() - 1; y >= 0; y--)
            {
                if (!buf)
                    buf = new tjs_uint8[bmppitch];
                if (pixelbytes == 4)
                {
                    TVPReverseRGB((tjs_uint32*)buf, (const tjs_uint32*)bmp->GetScanLineForRead(y),
                                  bmp->GetWidth());
                }
                else if (pixelbytes == 1)
                {
                    TVPDither32BitTo8Bit(buf, (const tjs_uint32*)bmp->GetScanLineForRead(y),
                                         bmp->GetWidth(), 0, y);
                }
                else
                {
                    const tjs_uint8* src = (const tjs_uint8*)bmp->GetScanLineForRead(y);
                    tjs_uint8* dest = buf;
                    tjs_int w = bmp->GetWidth();
                    for (tjs_int x = 0; x < w; x++)
                    {
                        dest[0] = src[2];
                        dest[1] = src[1];
                        dest[2] = src[0];
                        dest += 3;
                        src += 4;
                    }
                }
                stream->WriteBuffer(buf, bmppitch);
            }
        }
    }
    catch (...)
    {
        if (buf)
            delete[] buf;
        throw;
    }
    if (buf)
        delete[] buf;
}

void TVPSaveTextureAsBMP(const ttstr& path,
                         iTVPTexture2D* tex,
                         const ttstr& mode,
                         iTJSDispatch2* meta)
{
    tTJSBinaryStream* dst = TVPCreateStream(path, TJS_BS_WRITE);
    TVPSaveTextureAsBMP(dst, tex, mode, meta);
    delete dst;
}

void TVPSaveAsBMP(void* formatdata,
                  tTJSBinaryStream* dst,
                  const iTVPBaseBitmap* bmp,
                  const ttstr& mode,
                  iTJSDispatch2* meta)
{
    TVPSaveTextureAsBMP(dst, bmp->GetTexture(), mode, meta);
}
//---------------------------------------------------------------------------

void TVPLoadHeaderBMP(void* formatdata, tTJSBinaryStream* src, iTJSDispatch2** dic)
{
    tjs_uint64 firstpos = src->GetPosition();

    // check the magic
    tjs_uint8 magic[2];
    src->ReadBuffer(magic, 2);
    if (magic[0] != 'B' || magic[1] != 'M')
        TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPNotWindowsBmp);

    // read the BITMAPFILEHEADER
    TVP_WIN_BITMAPFILEHEADER bf;
    bf.bfSize = src->ReadI32LE();
    bf.bfReserved1 = src->ReadI16LE();
    bf.bfReserved2 = src->ReadI16LE();
    bf.bfOffBits = src->ReadI32LE();

    // read the BITMAPINFOHEADER
    TVP_WIN_BITMAPINFOHEADER bi;
    bi.biSize = src->ReadI32LE();
    if (bi.biSize == 12)
    {
        // OS/2 Bitmap
        memset(&bi, 0, sizeof(bi));
        bi.biWidth = (tjs_uint32)src->ReadI16LE();
        bi.biHeight = (tjs_uint32)src->ReadI16LE();
        bi.biPlanes = src->ReadI16LE();
        bi.biBitCount = src->ReadI16LE();
        bi.biClrUsed = 1 << bi.biBitCount;
    }
    else if (bi.biSize == 40)
    {
        // Windows Bitmap
        bi.biWidth = src->ReadI32LE();
        bi.biHeight = src->ReadI32LE();
        bi.biPlanes = src->ReadI16LE();
        bi.biBitCount = src->ReadI16LE();
        bi.biCompression = src->ReadI32LE();
        bi.biSizeImage = src->ReadI32LE();
        bi.biXPelsPerMeter = src->ReadI32LE();
        bi.biYPelsPerMeter = src->ReadI32LE();
        bi.biClrUsed = src->ReadI32LE();
        bi.biClrImportant = src->ReadI32LE();
    }
    else
    {
        TVPThrowExceptionMessage(TVPImageLoadError, (const tjs_char*)TVPUnsupportedHeaderVersion);
    }

    tjs_int palsize = (bi.biBitCount <= 8)
                          ? ((bi.biClrUsed == 0 ? (1 << bi.biBitCount) : bi.biClrUsed) *
                             ((bi.biSize == 12) ? 3 : 4))
                          : 0; // bi.biSize == 12 ( OS/2 palette )
    palsize = palsize > 0 ? 1 : 0;

    *dic = TJSCreateDictionaryObject();
    tTJSVariant val(bi.biWidth);
    (*dic)->PropSet(TJS_MEMBERENSURE, TJS_N("width"), 0, &val, (*dic));
    val = tTJSVariant(bi.biHeight);
    (*dic)->PropSet(TJS_MEMBERENSURE, TJS_N("height"), 0, &val, (*dic));
    val = tTJSVariant(bi.biBitCount);
    (*dic)->PropSet(TJS_MEMBERENSURE, TJS_N("bpp"), 0, &val, (*dic));
    val = tTJSVariant(palsize);
    (*dic)->PropSet(TJS_MEMBERENSURE, TJS_N("palette"), 0, &val, (*dic));
}

// export
tTVPRegisterGraphicInfo _bmpGraphicInfo(TJS_N(".bmp"),
                                        TVPQuickTestBMP,
                                        TVPLoadBMP,
                                        TVPLoadHeaderBMP,
                                        TVPSaveAsBMP,
                                        TVPAcceptSaveAsBMP,
                                        NULL);