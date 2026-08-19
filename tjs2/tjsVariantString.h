//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// string heap management used by tTJSVariant and tTJSString
//---------------------------------------------------------------------------
#ifndef tjsVariantStringH
#define tjsVariantStringH

#include "tjsConfig.h"
#include <stdlib.h>
#include <string.h>
#include <atomic>

namespace TJS
{

// #define TJS_DEBUG_UNRELEASED_STRING
// #define TJS_DEBUG_CHECK_STRING_HEAP_INTEGRITY
// #define TJS_DEBUG_DUMP_STRING

/*[*/
//---------------------------------------------------------------------------
// tTJSVariantString stuff
//---------------------------------------------------------------------------
#define TJS_VS_SHORT_LEN 21
/*]*/
class tTJSVariantString;
extern tjs_int TJSGetShorterStrLen(const tjs_char* str, tjs_int max);
extern tTJSVariantString* TJSAllocStringHeap(void);
extern void TJSDeallocStringHeap(tTJSVariantString* vs);
extern void TJSThrowStringAllocError();
extern void TJSThrowNarrowToWideConversionError();
extern void TJSCompactStringHeap();
#ifdef TJS_DEBUG_DUMP_STRING
extern void TJSDumpStringHeap(void);
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// base memory allocation functions for long string
//---------------------------------------------------------------------------
extern tjs_char* TJSVS_malloc(tjs_uint len);
extern tjs_char* TJSVS_realloc(tjs_char* buf, tjs_uint len);
extern void TJSVS_free(tjs_char* buf);
//---------------------------------------------------------------------------

/*[*/
//---------------------------------------------------------------------------
// tTJSVariantString
//---------------------------------------------------------------------------
struct tTJSVariantString_S
{
    // tjs_int RefCount; // reference count - 1
    std::atomic_long RefCount;
    tjs_char* LongString;
    tjs_char ShortString[TJS_VS_SHORT_LEN + 1];
    tjs_int Length; // string length
    tjs_uint32 HeapFlag;
    tjs_uint32 Hint;
};
/*]*/

/*start-of-tTJSVariantString*/
class tTJSVariantString : public tTJSVariantString_S
{
public:
    void AddRef() { RefCount++; }

    void Release();

    void SetString(const tjs_char* ref, tjs_int maxlen = -1)
    {
        if (LongString)
            TJSVS_free(LongString), LongString = NULL;
        tjs_int len;
        if (maxlen != -1)
            len = TJSGetShorterStrLen(ref, maxlen);
        else
            len = (tjs_int)TJS_strlen(ref);

        Length = len;
        if (len > TJS_VS_SHORT_LEN)
        {
            LongString = TJSVS_malloc(len + 1);
            TJS_strcpy_maxlen(LongString, ref, len);
        }
        else
        {
            TJS_strcpy_maxlen(ShortString, ref, len);
        }
    }

    void AllocBuffer(tjs_uint len)
    {
        /* note that you must call FixLength if you allocate larger than the
                actual string size */

        if (LongString)
            TJSVS_free(LongString), LongString = NULL;

        Length = len;
        if (len > TJS_VS_SHORT_LEN)
        {
            LongString = TJSVS_malloc(len + 1);
            LongString[len] = 0;
        }
        else
        {
            ShortString[len] = 0;
        }
    }

    void ResetString(const tjs_char* ref)
    {
        if (LongString)
            TJSVS_free(LongString), LongString = NULL;
        SetString(ref);
    }

    void AppendBuffer(tjs_uint applen)
    {
        /* note that you must call FixLength if you allocate larger than the
                actual string size */

        // assume this != NULL
        tjs_int newlen = Length += applen;
        if (LongString)
        {
            // still long string
            LongString = TJSVS_realloc(LongString, newlen + 1);
            LongString[newlen] = 0;
            return;
        }
        else
        {
            if (newlen <= TJS_VS_SHORT_LEN)
            {
                // still short string
                ShortString[newlen] = 0;
                return;
            }
            // becomes a long string
            tjs_char* newbuf = TJSVS_malloc(newlen + 1);
            TJS_strcpy(newbuf, ShortString);
            LongString = newbuf;
            LongString[newlen] = 0;
            return;
        }
    }

    void Append(const tjs_char* str)
    {
        // assume this != NULL
        Append(str, (tjs_int)TJS_strlen(str));
    }

    void Append(const tjs_char* str, tjs_int applen)
    {
        // assume this != NULL
        tjs_int orglen = Length;
        tjs_int newlen = Length += applen;
        if (LongString)
        {
            // still long string
            LongString = TJSVS_realloc(LongString, newlen + 1);
            TJS_strcpy(LongString + orglen, str);
            return;
        }
        else
        {
            if (newlen <= TJS_VS_SHORT_LEN)
            {
                // still short string
                TJS_strcpy(ShortString + orglen, str);
                return;
            }
            // becomes a long string
            tjs_char* newbuf = TJSVS_malloc(newlen + 1);
            TJS_strcpy(newbuf, ShortString);
            TJS_strcpy(newbuf + orglen, str);
            LongString = newbuf;
            return;
        }
    }

    operator const tjs_char*() const;

    tjs_int GetLength() const;
    tjs_int GetCharLength() const;

    tTJSVariantString* FixLength();

    tjs_uint32* GetHint() { return &Hint; }

    tTVInteger ToInteger() const;
    tTVReal ToReal() const;
    void ToNumber(tTJSVariant& dest) const;

    tjs_int GetRefCount() const { return RefCount; }

    tjs_int QueryPersistSize() const { return sizeof(tjs_uint) + GetLength() * sizeof(tjs_char); }

    void Persist(tjs_uint8* dest) const
    {
        tjs_uint size;
        const tjs_char* ptr = LongString ? LongString : ShortString;
        *(tjs_uint*)dest = size = GetLength();
        dest += sizeof(tjs_uint);
        while (size--)
        {
            *(tjs_char*)dest = *ptr;
            dest += sizeof(tjs_char);
            ptr++;
        }
    }
};
/*end-of-tTJSVariantString*/
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
extern tTJSVariantString* TJSAllocVariantString(const tjs_char* ref1, const tjs_char* ref2);
extern tTJSVariantString* TJSAllocVariantString(const tjs_char* ref, tjs_int n);
extern tTJSVariantString* TJSAllocVariantString(const tjs_char* ref);
extern tTJSVariantString* TJSAllocVariantString(const tjs_uint8** src);
extern tTJSVariantString* TJSAllocVariantStringBuffer(tjs_uint len);
extern tTJSVariantString* TJSAppendVariantString(tTJSVariantString* str, const tjs_char* app);
extern tTJSVariantString* TJSAppendVariantString(tTJSVariantString* str,
                                                 const tTJSVariantString* app);
extern tTJSVariantString* TJSFormatString(const tjs_char* format,
                                          tjs_uint numparams,
                                          tTJSVariant** params);

//---------------------------------------------------------------------------
} // namespace TJS
#endif
