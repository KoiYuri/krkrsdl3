//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

     See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Text read/write stream
//---------------------------------------------------------------------------
#include "tjsCommHead.h"
#include "TextStream.h"

#include "tjsError.h"
#include "UtilStreams.h"
#include "TVPMsg.h"
#include "TVPDebug.h"

#include <zlib.h>

/*
        Text stream is used by TJS's Array.save, Dictionary.saveStruct etc.
        to input/output text files.
*/

extern "C" int sjis_mbtowc(unsigned short* wc, const unsigned char* s);
extern "C" int gbk_mbtowc(unsigned short* wc, const unsigned char* s);

static int utf8_mbtowc(unsigned short* pwc, const unsigned char* s)
{
    unsigned char c = s[0];

    if (c < 0x80)
    {
        *pwc = c;
        return 1;
    }
    else if (c < 0xc2)
    {
        return -1;
    }
    else if (c < 0xe0)
    {
        if (!((s[1] ^ 0x80) < 0x40))
            return -1;
        *pwc = ((unsigned short)(c & 0x1f) << 6) | (unsigned short)(s[1] ^ 0x80);
        return 2;
    }
    else if (c < 0xf0)
    {
        if (!((s[1] ^ 0x80) < 0x40 && (s[2] ^ 0x80) < 0x40 && (c >= 0xe1 || s[1] >= 0xa0)))
            return -1;
        *pwc = ((unsigned short)(c & 0x0f) << 12) | ((unsigned short)(s[1] ^ 0x80) << 6) |
               (unsigned short)(s[2] ^ 0x80);
        return 3;
    }
    else
        return -1;
}

int (*mbtowc_for_text_stream)(unsigned short* wc, const unsigned char* s) = nullptr;

static size_t _TextStream_mbstowcs(int (*func_mbtowc)(unsigned short*, const unsigned char*),
                                   tjs_wchar* pwcs,
                                   const tjs_char* s,
                                   size_t n)
{
    if (!s)
        return -1;
    if (pwcs && n == 0)
        return 0;

    size_t count = 0;
    int cl;
    if (!pwcs)
    {
        while (*s)
        {
            unsigned short wc;
            cl = func_mbtowc(&wc, (const unsigned char*)s);
            if (cl <= 0)
                return -1;
            s += cl;
            ++count;
        }
    }
    else
    {
        while (*s && n > 0)
        {
            cl = func_mbtowc((unsigned short*)pwcs, (const unsigned char*)s);
            if (cl <= 0)
                return -1;
            n--;
            s += cl;
            pwcs++;
            ++count;
        }
    }
    return count;
}

// 宽容转换：遇到当前编码无法解码的孤立坏字节时，不再整篇失败，而是用替换字符
// U+FFFD 顶替并前进一个字节继续。out_errors 返回被替换的坏字节数量，供调用方在
// 多个候选编码里挑“最合身”（坏字节最少）的那个。永不返回 -1。
static const tjs_wchar TextStream_ReplacementChar = 0xFFFD;
static size_t _TextStream_mbstowcs_lenient(int (*func_mbtowc)(unsigned short*,
                                                              const unsigned char*),
                                           tjs_wchar* pwcs,
                                           const tjs_char* s,
                                           size_t n,
                                           size_t* out_errors)
{
    size_t count = 0;
    size_t errors = 0;
    int cl;
    if (!s)
    {
        if (out_errors)
            *out_errors = (size_t)-1;
        return (size_t)-1;
    }
    if (!pwcs)
    {
        while (*s)
        {
            unsigned short wc;
            cl = func_mbtowc(&wc, (const unsigned char*)s);
            if (cl <= 0)
            {
                ++errors;
                s += 1;
            }
            else
            {
                s += cl;
            }
            ++count;
        }
    }
    else
    {
        while (*s && n > 0)
        {
            cl = func_mbtowc((unsigned short*)pwcs, (const unsigned char*)s);
            if (cl <= 0)
            {
                *(unsigned short*)pwcs = TextStream_ReplacementChar;
                ++errors;
                s += 1;
            }
            else
            {
                s += cl;
            }
            n--;
            pwcs++;
            ++count;
        }
    }
    if (out_errors)
        *out_errors = errors;
    return count;
}

// 所有编码都无法整篇干净解码时，选一个坏字节最少的编码来做宽容解码。
// 选择过程只依赖输入串本身，因此“量长度”和“真正写入”两趟必然得到一致结果。
static int (*_TextStream_pick_lenient_encoding(const tjs_char* s))(unsigned short*,
                                                                   const unsigned char*)
{
    int (*candidates[3])(unsigned short*, const unsigned char*) = { utf8_mbtowc, sjis_mbtowc,
                                                                    gbk_mbtowc };
    int (*best)(unsigned short*, const unsigned char*) = gbk_mbtowc;
    size_t best_errors = (size_t)-1;
    for (int i = 0; i < 3; i++)
    {
        size_t errs = 0;
        _TextStream_mbstowcs_lenient(candidates[i], nullptr, s, 0, &errs);
        if (errs < best_errors)
        {
            best_errors = errs;
            best = candidates[i];
        }
    }
    return best;
}

size_t TextStream_mbstowcs(tjs_wchar* pwcs, const tjs_char* s, size_t n)
{
    if (mbtowc_for_text_stream)
    {
        // 脚本通过 Storages.setTextEncoding 指定编码时，始终服从脚本的明确设置。
        return _TextStream_mbstowcs(mbtowc_for_text_stream, pwcs, s, n);
    }

    // 无 BOM 文本不能把第一次探测结果缓存到全局。
    // 同一个游戏可能同时包含 Shift-JIS、UTF-8 和 UTF-16 脚本；如果前一个
    // 文件恰好只有 ASCII 或是日文，后续汉化脚本就会被错误地按同一编码读取。
    // UTF-8 优先可以正确处理汉化脚本，失败后再兼容原版日文和 GBK 脚本。
    size_t ret = _TextStream_mbstowcs(utf8_mbtowc, pwcs, s, n);
    if (ret != (size_t)-1)
        return ret;
    ret = _TextStream_mbstowcs(sjis_mbtowc, pwcs, s, n);
    if (ret != (size_t)-1)
        return ret;
    ret = _TextStream_mbstowcs(gbk_mbtowc, pwcs, s, n);
    if (ret != (size_t)-1)
        return ret;

    // 没有任何编码能整篇干净解码。手工汉化脚本常见这种情况：整篇是规整的 GBK，
    // 但夹着一两个残缺字节（例如 [w] 标签旁遗留了半个全角标点的首字节）。原来的
    // 做法是整篇判失败，最终抛出 “Cannot convert given narrow string to wide
    // string”，玩家看到的就是 jump 时报错卡死。这里改为选最合身的编码做宽容解码，
    // 只把那一两个坏字节替换掉，其余正文照常显示。
    int (*best)(unsigned short*, const unsigned char*) = _TextStream_pick_lenient_encoding(s);
    return _TextStream_mbstowcs_lenient(best, pwcs, s, n, nullptr);
}

static ttstr enc_utf8 = TJS_N("utf8"), enc_utf8_2 = TJS_N("utf-8"), enc_utf16 = TJS_N("utf16"),
             enc_utf16_2 = TJS_N("utf-16"), enc_gbk = TJS_N("gbk"), enc_jis = TJS_N("sjis"),
             enc_jis_2 = TJS_N("shiftjis"), enc_jis_3 = TJS_N("shift_jis"),
             enc_jis_4 = TJS_N("shift-jis");

static ttstr DefaultReadEncoding = TJS_N("UTF-8");
//---------------------------------------------------------------------------
// Interface to tTJSTextStream
//---------------------------------------------------------------------------
/*
        note: encryption of mode 0 or 1 ( simple crypt ) does never intend data pretection
                  security.
*/
class tTVPTextReadStream : public iTJSTextReadStream
{
    tTJSBinaryStream* Stream;
    bool DirectLoad;
    tjs_char* Buffer;
    size_t BufferLen;
    tjs_char* BufferPtr;
    tjs_int CryptMode;

public:
    tTVPTextReadStream(tTJSBinaryStream* _stream, const ttstr& modestr)
    {
        // following syntax of modestr is currently supported.
        // oN: read from binary offset N (in bytes)

#ifndef TVP_NO_CHECK_WIDE_CHAR_SIZE
        if (sizeof(tjs_wchar) != 2)
            TVPThrowExceptionMessage(TVPTheHostIsNotA16BitUnicodeSystem);
#endif

        Stream = NULL;
        Buffer = NULL;
        DirectLoad = false;
        CryptMode = -1;

        // check o mode
        Stream = _stream;

        tjs_uint64 ofs = 0;
        const tjs_char* o_ofs;
        o_ofs = TJS_strchr(modestr.c_str(), TJS_N('o'));
        if (o_ofs != NULL)
        {
            // seek to offset
            o_ofs++;
            tjs_char buf[256];
            int i;
            for (i = 0; i < 255; i++)
            {
                if (o_ofs[i] >= TJS_N('0') && o_ofs[i] <= TJS_N('9'))
                    buf[i] = o_ofs[i];
                else
                    break;
            }
            buf[i] = 0;
            ofs = ttstr(buf).AsInteger();
            Stream->SetPosition(ofs);
        }

        // check first of the file - whether the file is unicode
        try
        {
            tjs_uint8 mark[3] = {0, 0};
            Stream->Read(mark, 3);
            if (mark[0] == 0xff && mark[1] == 0xfe)
            {
                // unicode
                Stream->SetPosition(ofs + 2);
                DirectLoad = true;
            }
            else if (mark[0] == 0xfe && mark[1] == 0xfe)
            {
                // ciphered text or compressed
                tjs_uint8 mode = mark[2];
                if (mode != 0 && mode != 1 && mode != 2)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, "");
                // currently only mode0 and mode1, and compressed (mode2) are supported
                CryptMode = mode;
                DirectLoad = CryptMode != 2;

                Stream->Read(mark, 2); // original bom code comes here (is not compressed)
                if (mark[0] != 0xff || mark[1] != 0xfe)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, "");

                if (CryptMode == 2)
                {
                    // compressed text stream
                    tjs_uint64 compressed = Stream->ReadI64LE();
                    tjs_uint64 uncompressed = Stream->ReadI64LE();
                    if (compressed != (unsigned long)compressed ||
                        uncompressed != (unsigned long)uncompressed)
                        TVPThrowExceptionMessage(TVPUnsupportedCipherMode, "");
                    // too large stream
                    unsigned long destlen;
                    tjs_uint8* nbuf = new tjs_uint8[(unsigned long)compressed + 1];
                    tjs_uint8* unbuf = new tjs_uint8[(destlen = (unsigned long)uncompressed) + 1];
                    try
                    {
                        Stream->ReadBuffer(nbuf, (unsigned long)compressed);
                        int result = uncompress(/* uncompress from zlib */
                                                (unsigned char*)unbuf, &destlen,
                                                (unsigned char*)nbuf, (unsigned long)compressed);
                        if (result != Z_OK || destlen != (unsigned long)uncompressed)
                            TVPThrowExceptionMessage(TVPUnsupportedCipherMode, "");

                        // convert
                        BufferLen = TVPWideCharToUtf8String((tjs_wchar*)nbuf, NULL);
                        if (BufferLen == (size_t)-1)
                            TVPThrowExceptionMessage(TJSWideToNarrowConversionError);
                        Buffer = new tjs_char[BufferLen + 1];
                        TVPWideCharToUtf8String((tjs_wchar*)nbuf, Buffer);
                    }
                    catch (...)
                    {
                        delete[] nbuf;
                        delete[] unbuf;
                        throw;
                    }
                    delete[] nbuf;
                    delete[] unbuf;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
            }
            else
            {
                // check UTF-8 BOM
                if (mark[0] == 0xef && mark[1] == 0xbb && mark[2] == 0xbf)
                {
                    // UTF-8 BOM
                    BufferLen = (tjs_uint)(Stream->GetSize() - 3) - ofs;
                    Buffer = new tjs_char[BufferLen + 1];
                    try
                    {
                        Stream->ReadBuffer(Buffer, BufferLen);
                    }
                    catch (...)
                    {
                        throw;
                    }
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
                else
                {
                    // ansi/mbcs
                    // read whole and hold it
                    Stream->SetPosition(ofs);
                    tjs_uint size = (tjs_uint)(Stream->GetSize()) - ofs;
                    tjs_uint8* nbuf = new tjs_uint8[size + 1];
                    tjs_wchar* interBuff = nullptr;
                    try
                    {
                        Stream->ReadBuffer(nbuf, size);
                        nbuf[size] = 0; // terminater
                        // 闲的慌啊？以后会优化的 TODO
                        // to utf-16
                        size_t interSize = TextStream_mbstowcs(NULL, (tjs_char*)nbuf, 0);
                        if (interSize == (size_t)-1)
                            TVPThrowExceptionMessage(TJSNarrowToWideConversionError);
                        interBuff = new tjs_wchar[interSize + 1];
                        TextStream_mbstowcs(interBuff, (tjs_char*)nbuf, interSize);
                        interBuff[interSize] = 0;
                        // to utf-8
                        BufferLen = TVPWideCharToUtf8String(interBuff, NULL);
                        if (BufferLen == (size_t)-1)
                            TVPThrowExceptionMessage(TJSWideToNarrowConversionError);
                        Buffer = new tjs_char[BufferLen + 1];
                        TVPWideCharToUtf8String(interBuff, Buffer);
                    }
                    catch (...)
                    {
                        delete[] nbuf;
                        if (interBuff)
                            delete[] interBuff;
                        throw;
                    }
                    delete[] nbuf;
                    if (interBuff)
                        delete[] interBuff;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
            }
        }
        catch (...)
        {
            delete Stream;
            Stream = NULL;
            throw;
        }
    }
    tTVPTextReadStream(const ttstr& name, const ttstr& modestr)
    {
        // following syntax of modestr is currently supported.
        // oN: read from binary offset N (in bytes)

#ifndef TVP_NO_CHECK_WIDE_CHAR_SIZE
        if (sizeof(tjs_wchar) != 2)
            TVPThrowExceptionMessage(TVPTheHostIsNotA16BitUnicodeSystem);
#endif

        Stream = NULL;
        Buffer = NULL;
        DirectLoad = false;
        CryptMode = -1;

        // check o mode
        Stream = TVPCreateStream(name, TJS_BS_READ);

        tjs_uint64 ofs = 0;
        const tjs_char* o_ofs;
        o_ofs = TJS_strchr(modestr.c_str(), TJS_N('o'));
        if (o_ofs != NULL)
        {
            // seek to offset
            o_ofs++;
            tjs_char buf[256];
            int i;
            for (i = 0; i < 255; i++)
            {
                if (o_ofs[i] >= TJS_N('0') && o_ofs[i] <= TJS_N('9'))
                    buf[i] = o_ofs[i];
                else
                    break;
            }
            buf[i] = 0;
            ofs = ttstr(buf).AsInteger();
            Stream->SetPosition(ofs);
        }

        // check first of the file - whether the file is unicode
        try
        {
            tjs_uint8 mark[3] = {0, 0};
            Stream->Read(mark, 3);
            if (mark[0] == 0xff && mark[1] == 0xfe)
            {
                // unicode
                Stream->SetPosition(ofs + 2);
                DirectLoad = true;
            }
            else if (mark[0] == 0xfe && mark[1] == 0xfe)
            {
                // ciphered text or compressed
                tjs_uint8 mode = mark[2];
                if (mode != 0 && mode != 1 && mode != 2)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);
                // currently only mode0 and mode1, and compressed (mode2) are supported
                CryptMode = mode;
                DirectLoad = CryptMode != 2;

                Stream->Read(mark, 2); // original bom code comes here (is not compressed)
                if (mark[0] != 0xff || mark[1] != 0xfe)
                    TVPThrowExceptionMessage(TVPUnsupportedCipherMode, name);

                if (CryptMode == 2)
                {
                    // compressed text stream
                    tjs_uint64 compressed = Stream->ReadI64LE();
                    tjs_uint64 uncompressed = Stream->ReadI64LE();
                    if (compressed != (unsigned long)compressed ||
                        uncompressed != (unsigned long)uncompressed)
                        TVPThrowExceptionMessage(TVPUnsupportedCipherMode, "");
                    // too large stream
                    unsigned long destlen;
                    tjs_uint8* nbuf = new tjs_uint8[(unsigned long)compressed + 1];
                    tjs_uint16* unbuf =
                        new tjs_uint16[(destlen = (unsigned long)uncompressed) / 2 + 1];
                    try
                    {
                        Stream->ReadBuffer(nbuf, (unsigned long)compressed);
                        int result = uncompress(/* uncompress from zlib */
                                                (unsigned char*)unbuf, &destlen,
                                                (unsigned char*)nbuf, (unsigned long)compressed);
                        if (result != Z_OK || destlen != (unsigned long)uncompressed)
                            TVPThrowExceptionMessage(TVPUnsupportedCipherMode, "");
                        unbuf[destlen / 2] = 0;
                        // convert
                        BufferLen = TVPWideCharToUtf8String((tjs_wchar*)unbuf, NULL);
                        if (BufferLen == (size_t)-1)
                            TVPThrowExceptionMessage(TJSWideToNarrowConversionError);
                        Buffer = new tjs_char[BufferLen + 1];
                        TVPWideCharToUtf8String((tjs_wchar*)unbuf, Buffer);
                    }
                    catch (...)
                    {
                        delete[] nbuf;
                        delete[] unbuf;
                        throw;
                    }
                    delete[] nbuf;
                    delete[] unbuf;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
            }
            else
            {
                // check UTF-8 BOM
                if (mark[0] == 0xef && mark[1] == 0xbb && mark[2] == 0xbf)
                {
                    // UTF-8 BOM
                    BufferLen = (tjs_uint)(Stream->GetSize() - 3) - ofs;
                    Buffer = new tjs_char[BufferLen + 1];
                    try
                    {
                        Stream->ReadBuffer(Buffer, BufferLen);
                    }
                    catch (...)
                    {
                        throw;
                    }
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
                else
                {
                    // ansi/mbcs
                    // read whole and hold it
                    Stream->SetPosition(ofs);
                    tjs_uint size = (tjs_uint)(Stream->GetSize()) - ofs;
                    tjs_uint8* nbuf = new tjs_uint8[size + 1];
                    tjs_wchar* interBuff = nullptr;
                    try
                    {
                        Stream->ReadBuffer(nbuf, size);
                        nbuf[size] = 0;
                        // terminater
                        // 闲的慌啊？以后会优化的 TODO
                        // to utf-16
                        size_t interSize = TextStream_mbstowcs(NULL, (tjs_char*)nbuf, 0);
                        if (interSize == (size_t)-1)
                            TVPThrowExceptionMessage(TJSNarrowToWideConversionError);
                        interBuff = new tjs_wchar[interSize + 1];
                        TextStream_mbstowcs(interBuff, (tjs_char*)nbuf, interSize);
                        interBuff[interSize] = 0;
                        // to utf-8
                        BufferLen = TVPWideCharToUtf8String(interBuff, NULL);
                        if (BufferLen == (size_t)-1)
                            TVPThrowExceptionMessage(TJSWideToNarrowConversionError);
                        Buffer = new tjs_char[BufferLen + 1];
                        TVPWideCharToUtf8String(interBuff, Buffer);
                    }
                    catch (...)
                    {
                        delete[] nbuf;
                        if (interBuff)
                            delete[] interBuff;
                        throw;
                    }
                    delete[] nbuf;
                    if (interBuff)
                        delete[] interBuff;
                    Buffer[BufferLen] = 0;
                    BufferPtr = Buffer;
                }
            }
        }
        catch (...)
        {
            delete Stream;
            Stream = NULL;
            throw;
        }
    }

    ~tTVPTextReadStream()
    {
        if (Stream)
            delete Stream;
        if (Buffer)
            delete[] Buffer;
    }

    tjs_uint Read(tTJSString& targ, tjs_uint size)
    {
        if (DirectLoad)
        {
            if (size == 0)
                size = static_cast<tjs_uint>(Stream->GetSize() - Stream->GetPosition());
            if (!size)
            {
                targ.Clear();
                return 0;
            }
            tjs_wchar* buf = new tjs_wchar[size * 2 + 1];
            tjs_uint read = Stream->Read(buf, size * 2); // 2 = BMP unicode size
            read /= 2;
#if TJS_HOST_IS_BIG_ENDIAN
            // re-order input
            for (tjs_uint i = 0; i < read; i++)
            {
                tjs_wchar ch = buf[i];
                buf[i] = ((ch >> 8) & 0xff) + ((ch & 0xff) << 8);
            }
#endif
            if (CryptMode == 0)
            {
                // simple crypt
                for (tjs_uint i = 0; i < read; i++)
                {
                    tjs_wchar ch = buf[i];
                    if (ch >= 0x20)
                        buf[i] = ch ^ (((ch & 0xfe) << 8) ^ 1);
                }
            }
            else if (CryptMode == 1)
            {
                // simple crypt
                for (tjs_uint i = 0; i < read; i++)
                {
                    tjs_wchar ch = buf[i];
                    ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                    buf[i] = ch;
                }
            }
            buf[read] = 0;

            // convert
            BufferLen = TVPWideCharToUtf8String(buf, NULL);
            if (BufferLen == (size_t)-1)
                TVPThrowExceptionMessage(TJSWideToNarrowConversionError);
            tjs_char* bufS = targ.AllocBuffer(BufferLen);
            TVPWideCharToUtf8String(buf, bufS);
            bufS[BufferLen] = 0;
            delete[] buf;
            targ.FixLen();
            return read;
        }
        else
        {
            if (size == 0)
                size = (tjs_uint)BufferLen;
            if (size)
            {
                tjs_char* buf = targ.AllocBuffer(size);
                TJS_strncpy(buf, BufferPtr, size);
                buf[size] = 0;
                BufferPtr += size;
                BufferLen -= size;
                targ.FixLen();
            }
            else
            {
                targ.Clear();
            }
            return size;
        }
    }

    void Destruct() { delete this; }
};
//---------------------------------------------------------------------------
class tTVPTextWriteStream : public iTJSTextWriteStream
{
    // TODO: 32bit wchar_t support

    static const tjs_uint COMPRESSION_BUFFER_SIZE = 1024 * 1024;

    tTJSBinaryStream* Stream;
    tjs_int CryptMode;
    // -1 for no-crypt
    // 0: (unused)	(old buggy crypt mode)
    // 1: simple crypt
    // 2: complessed
    int CompressionLevel; // compression level of zlib

    z_stream_s* ZStream;
    tjs_uint CompressionSizePosition;
    tjs_char* CompressionBuffer;
    bool CompressionFailed;

public:
    tTVPTextWriteStream(const ttstr& name, const ttstr& modestr)
    {
        // modestr supports following modes:
        // dN: deflate(compress) at mode N ( currently not implemented )
        // cN: write in cipher at mode N ( currently n is ignored )
        // zN: write with compress at mode N ( N is compression level )
        // oN: write from binary offset N (in bytes)
        Stream = NULL;
        CryptMode = -1;
        CompressionLevel = Z_DEFAULT_COMPRESSION;
        ZStream = NULL;
        CompressionSizePosition = 0;
        CompressionBuffer = NULL;
        CompressionFailed = false;

        // check c/z mode
        const tjs_char* p;
        if ((p = TJS_strchr(modestr.c_str(), TJS_N('c'))) != NULL)
        {
            CryptMode = 1; // simple crypt
            if (p[1] >= TJS_N('0') && p[1] <= TJS_N('9'))
                CryptMode = p[1] - TJS_N('0');
        }

        if ((p = TJS_strchr(modestr.c_str(), TJS_N('z'))) != NULL)
        {
            CryptMode = 2; // compressed (cannot be with 'c')
            if (p[1] >= TJS_N('0') && p[1] <= TJS_N('9'))
                CompressionLevel = p[1] - TJS_N('0');
        }

        if (CryptMode != -1 && CryptMode != 1 && CryptMode != 2)
            TVPThrowExceptionMessage(TVPUnsupportedModeString, TJS_N("unsupported cipher mode"));

        // check o mode
        const tjs_char* o_ofs;
        o_ofs = TJS_strchr(modestr.c_str(), TJS_N('o'));
        if (o_ofs != NULL)
        {
            // seek to offset
            o_ofs++;
            tjs_char buf[256];
            int i;
            for (i = 0; i < 255; i++)
            {
                if (o_ofs[i] >= TJS_N('0') && o_ofs[i] <= TJS_N('9'))
                    buf[i] = o_ofs[i];
                else
                    break;
            }
            buf[i] = 0;
            tjs_uint64 ofs = ttstr(buf).AsInteger();
            Stream = TVPCreateStream(name, TJS_BS_UPDATE);
            Stream->SetPosition(ofs);
        }
        else
        {
            Stream = TVPCreateStream(name, TJS_BS_WRITE);
        }

        if (CryptMode == 1 || CryptMode == 2)
        {
            // simple crypt or compressed
            tjs_uint8 crypt_mode_sig[4];
            crypt_mode_sig[0] = crypt_mode_sig[1] = 0xfe;
            crypt_mode_sig[2] = (tjs_uint8)CryptMode;
            crypt_mode_sig[3] = 0;
            Stream->WriteBuffer(crypt_mode_sig, 3);
        }

        // now output text stream will write unicode texts
        static tjs_uint8 bommark[4] = {0xff, 0xfe, 0x00, 0x00 /*dummy 2bytes*/};
        Stream->WriteBuffer(bommark, 2);

        if (CryptMode == 2)
        {
            // allocate and initialize zlib straem
            ZStream = new z_stream_s();
            ZStream->zalloc = Z_NULL;
            ZStream->zfree = Z_NULL;
            ZStream->opaque = Z_NULL;
            if (deflateInit(ZStream, CompressionLevel) != Z_OK)
            {
                CompressionFailed = true;
                TVPThrowExceptionMessage(TVPCompressionFailed);
            }

            CompressionBuffer = new tjs_char[COMPRESSION_BUFFER_SIZE];

            ZStream->next_in = NULL;
            ZStream->avail_in = 0;
            ZStream->next_out = reinterpret_cast<Bytef*>(CompressionBuffer);
            ZStream->avail_out = COMPRESSION_BUFFER_SIZE;

            // Compression Size (write dummy)
            CompressionSizePosition = static_cast<tjs_uint>(Stream->GetPosition());
            WriteI64LE((tjs_uint64)0);
            WriteI64LE((tjs_uint64)0);
        }
    }

    ~tTVPTextWriteStream()
    {
        if (CryptMode == 2)
        {

            if (!CompressionFailed)
            {
                try
                {
                    // close zlib stream
                    int result = 0;
                    do
                    {
                        result = deflate(ZStream, Z_FINISH);
                        if (result != Z_OK && result != Z_STREAM_END)
                        {
                            TVPThrowExceptionMessage(TVPCompressionFailed);
                        }
                        Stream->WriteBuffer(CompressionBuffer,
                                            COMPRESSION_BUFFER_SIZE - ZStream->avail_out);
                        ZStream->next_out = reinterpret_cast<Bytef*>(CompressionBuffer);
                        ZStream->avail_out = COMPRESSION_BUFFER_SIZE;
                    } while (result != Z_STREAM_END);

                    // rollback and write compression size.
                    Stream->SetPosition(CompressionSizePosition);
                    WriteI64LE((tjs_uint64)ZStream->total_out);
                    WriteI64LE((tjs_uint64)ZStream->total_in);
                }
                catch (...)
                {
                    // delete zlib compress stream
                    if (ZStream)
                    {
                        deflateEnd(ZStream);
                        delete ZStream;
                    }
                    delete[] CompressionBuffer;
                    delete Stream;
                    return;
                }
            }
            // delete zlib compress stream
            if (ZStream)
            {
                deflateEnd(ZStream);
                delete ZStream;
            }
            delete[] CompressionBuffer;
        }

        if (Stream)
            delete Stream;
    }

    void WriteI64LE(tjs_uint64 v)
    {
        // write 64bit little endian value to the file.
        tjs_uint8 buf[8];
        buf[0] = (tjs_uint8)(v >> (0 * 8));
        buf[1] = (tjs_uint8)(v >> (1 * 8));
        buf[2] = (tjs_uint8)(v >> (2 * 8));
        buf[3] = (tjs_uint8)(v >> (3 * 8));
        buf[4] = (tjs_uint8)(v >> (4 * 8));
        buf[5] = (tjs_uint8)(v >> (5 * 8));
        buf[6] = (tjs_uint8)(v >> (6 * 8));
        buf[7] = (tjs_uint8)(v >> (7 * 8));

        Stream->WriteBuffer(buf, 8);
    }

    void Write(const ttstr& targ)
    {
        tjs_wchar* buf = NULL;
        tjs_int len = 0;
        try
        {
            len = TVPUtf8ToWideCharString(targ.c_str(), NULL);
            if (len <= 0)
                return;
            buf = new tjs_wchar[len + 1];
            TVPUtf8ToWideCharString(targ.c_str(), buf);
            buf[len] = 0;

#if TJS_HOST_IS_BIG_ENDIAN
            tjs_uint16* p;
            if (CryptMode == 1)
            {
                // simple crypt
                p = buf;
                if (p)
                {
                    while (*p)
                    {
                        tjs_char ch = *p;
                        ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                        *p = ch;
                        p++;
                    }
                }
            }

            // re-order to little endian unicode text
            p = buf;
            if (p)
            {
                while (*p)
                {
                    *p = ((*p >> 8) & 0xff) + ((*p & 0xff) << 8);
                    p++;
                }
            }

            WriteRawData(str.c_str(), len * sizeof(tjs_char));
#else
            if (CryptMode == 1)
            {
                // simple crypt
                tjs_wchar* p;
                p = buf;
                if (p)
                {
                    while (*p)
                    {
                        tjs_wchar ch = *p;
                        ch = ((ch & 0xaaaaaaaa) >> 1) | ((ch & 0x55555555) << 1);
                        *p = ch;
                        p++;
                    }
                }

                WriteRawData(buf, len * sizeof(tjs_wchar));
            }
            else
            {
                WriteRawData(buf, len * sizeof(tjs_wchar));
            }
#endif
        }
        catch (...)
        {
            delete[] buf;
            throw;
        }
        delete[] buf;
    }

    void WriteRawData(void* ptr, size_t size)
    {
        if (CryptMode == 2)
        {
            // compressed with zlib stream.
            ZStream->next_in = (Bytef*)ptr;
            ZStream->avail_in = (uInt)size;

            while (ZStream->avail_in > 0)
            {
                int result = deflate(ZStream, Z_NO_FLUSH);
                if (result != Z_OK)
                {
                    CompressionFailed = true;
                    TVPThrowExceptionMessage(TVPCompressionFailed);
                }
                if (ZStream->avail_out == 0)
                {
                    Stream->WriteBuffer(CompressionBuffer, COMPRESSION_BUFFER_SIZE);
                    ZStream->next_out = reinterpret_cast<Bytef*>(CompressionBuffer);
                    ZStream->avail_out = COMPRESSION_BUFFER_SIZE;
                }
            }
        }
        else
        {
            Stream->WriteBuffer(ptr, (tjs_uint)size); // write directly
        }
    }

    void Destruct() { delete this; }
};

//---------------------------------------------------------------------------
iTJSTextReadStream* TVPCreateTextStreamForRead(tTJSBinaryStream* stream, const ttstr& modestr)
{
    return new tTVPTextReadStream(stream, modestr);
}
//---------------------------------------------------------------------------
iTJSTextReadStream* TVPCreateTextStreamForRead(const ttstr& name, const ttstr& modestr)
{
    return new tTVPTextReadStream(name, modestr);
}
//---------------------------------------------------------------------------
iTJSTextWriteStream* TVPCreateTextStreamForWrite(const ttstr& name, const ttstr& modestr)
{
    return new tTVPTextWriteStream(name, modestr);
}
//---------------------------------------------------------------------------
void TVPSetDefaultReadEncoding(const ttstr& encoding)
{
    DefaultReadEncoding = encoding;
    ttstr codestr = encoding;
    codestr.ToLowerCase();
    if (codestr == enc_gbk)
    {
        mbtowc_for_text_stream = gbk_mbtowc;
    }
    else if (codestr == enc_utf8 || codestr == enc_utf8_2)
    {
        mbtowc_for_text_stream = utf8_mbtowc;
    }
    else if (codestr == enc_jis || codestr == enc_jis_2 || codestr == enc_jis_3 ||
             codestr == enc_jis_4)
    {
        mbtowc_for_text_stream = sjis_mbtowc;
    }
    else
    {
        TVPThrowExceptionMessage(TVPUnsupportedEncoding, encoding);
    }
}
//---------------------------------------------------------------------------
const tjs_char* TVPGetDefaultReadEncoding()
{
    return DefaultReadEncoding.c_str();
}
//---------------------------------------------------------------------------
