//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// tTJSVariant friendly string class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <stdarg.h>
#include <stdio.h>

#include "tjsString.h"
#include "tjsVariant.h"

#define TJS_TTSTR_SPRINTF_BUF_SIZE 8192

namespace TJS
{
const tjs_char* TJSNullStrPtr = TJS_N("");
//---------------------------------------------------------------------------
tTJSString::tTJSString(const tTJSVariant& val)
{
    Ptr = val.AsString();
}
//---------------------------------------------------------------------------
tTJSString::tTJSString(tjs_int n) // from int
{
    Ptr = TJSIntegerToString(n);
}
//---------------------------------------------------------------------------
tTJSString::tTJSString(tjs_int64 n) // from int64
{
    Ptr = TJSIntegerToString(n);
}
//---------------------------------------------------------------------------
tjs_char* tTJSString::InternalIndepend()
{
    // severs sharing of the string instance
    // and returns independent internal buffer

    tTJSVariantString* newstr = TJSAllocVariantString(Ptr ? (const tjs_char*)(*Ptr) : NULL);

    Ptr->Release();
    Ptr = newstr;

    return const_cast<tjs_char*>(newstr ? (const tjs_char*)(*newstr) : NULL);
}
//---------------------------------------------------------------------------
tjs_int64 tTJSString::AsInteger() const
{
    if (!Ptr)
        return 0;
    return Ptr->ToInteger();
}
//---------------------------------------------------------------------------
void tTJSString::Replace(const tTJSString& from, const tTJSString& to, bool forall)
{
    // replaces the string partial "from", to "to".
    // all "from" are replaced when "forall" is true.
    if (IsEmpty())
        return;
    if (from.IsEmpty())
        return;

    tjs_int fromlen = from.GetLen();

    for (;;)
    {
        const tjs_char* st;
        const tjs_char* p;
        st = c_str();
        p = TJS_strstr(st, from.c_str());
        if (p)
        {
            tTJSString name(*this, (int)(p - st));
            tTJSString n2(p + fromlen);
            *this = name + to + n2;
            if (!forall)
                break;
        }
        else
        {
            break;
        }
    }
}
//---------------------------------------------------------------------------
tTJSString tTJSString::AsLowerCase() const
{
    tjs_int len = GetLen();

    if (len == 0)
        return tTJSString();

    tTJSString ret((tTJSStringBufferLength)(len));

    const tjs_char* s = c_str();
    tjs_char* d = ret.Independ();
    while (*s)
    {
        if (*s >= TJS_N('A') && *s <= TJS_N('Z'))
            *d = *s + (TJS_N('a') - TJS_N('A'));
        else
            *d = *s;
        d++;
        s++;
    }

    return ret;
}
//---------------------------------------------------------------------------
tTJSString tTJSString::AsUpperCase() const
{
    tjs_int len = GetLen();

    if (len == 0)
        return tTJSString();

    tTJSString ret((tTJSStringBufferLength)(len));

    const tjs_char* s = c_str();
    tjs_char* d = ret.Independ();
    while (*s)
    {
        if (*s >= TJS_N('a') && *s <= TJS_N('z'))
            *d = *s + (TJS_N('A') - TJS_N('a'));
        else
            *d = *s;
        d++;
        s++;
    }

    return ret;
}
//---------------------------------------------------------------------------
void tTJSString::ToLowerCase()
{
    tjs_char* p = Independ();
    if (p)
    {
        while (*p)
        {
            if (*p >= TJS_N('A') && *p <= TJS_N('Z'))
                *p += (TJS_N('a') - TJS_N('A'));
            p++;
        }
    }
}
//---------------------------------------------------------------------------
void tTJSString::ToUppserCase()
{
    tjs_char* p = Independ();
    if (p)
    {
        while (*p)
        {
            if (*p >= TJS_N('a') && *p <= TJS_N('z'))
                *p += (TJS_N('A') - TJS_N('a'));
            p++;
        }
    }
}
//---------------------------------------------------------------------------
tjs_int tTJSString::printf(const tjs_char* format, ...)
{
    tjs_int r;
    tjs_char* buf = new tjs_char[TJS_TTSTR_SPRINTF_BUF_SIZE];
    try
    {
        tjs_int size = TJS_TTSTR_SPRINTF_BUF_SIZE - 1; /*TJS_vsnprintf(NULL, 0, format, param);*/
        va_list param;
        va_start(param, format);
        r = TJS_vsnprintf(buf, size, format, param);
        AllocBuffer(r);
        if (r)
        {
            TJS_strcpy(const_cast<tjs_char*>(c_str()), buf);
        }
        va_end(param);
        FixLen();
    }
    catch (...)
    {
        delete[] buf;
        throw;
    }
    delete[] buf;
    return r;
}
//---------------------------------------------------------------------------
tTJSString tTJSString::EscapeC() const
{
    ttstr ret;
    const tjs_char* p = c_str();
    bool hexflag = false;
    for (; *p; p++)
    {
        switch (*p)
        {
            case 0x07:
                ret += TJS_N("\\a");
                hexflag = false;
                continue;
            case 0x08:
                ret += TJS_N("\\b");
                hexflag = false;
                continue;
            case 0x0c:
                ret += TJS_N("\\f");
                hexflag = false;
                continue;
            case 0x0a:
                ret += TJS_N("\\n");
                hexflag = false;
                continue;
            case 0x0d:
                ret += TJS_N("\\r");
                hexflag = false;
                continue;
            case 0x09:
                ret += TJS_N("\\t");
                hexflag = false;
                continue;
            case 0x0b:
                ret += TJS_N("\\v");
                hexflag = false;
                continue;
            case TJS_N('\\'):
                ret += TJS_N("\\\\");
                hexflag = false;
                continue;
            case TJS_N('\''):
                ret += TJS_N("\\\'");
                hexflag = false;
                continue;
            case TJS_N('\"'):
                ret += TJS_N("\\\"");
                hexflag = false;
                continue;
            default:
                int chLen = utf8_char_len(p);
                if (chLen == 0)
                    continue;
                if (hexflag)
                {
                    if (chLen == 1 && ((*p >= TJS_N('a') && *p <= TJS_N('f')) ||
                                       (*p >= TJS_N('A') && *p <= TJS_N('F')) ||
                                       (*p >= TJS_N('0') && *p <= TJS_N('9'))))
                    {
                        tjs_char buf[20];
                        TJS_snprintf(buf, sizeof(buf) / sizeof(tjs_char), TJS_N("\\x%02x"),
                                     (int)*p);
                        hexflag = true;
                        ret += buf;
                        continue;
                    }
                }
                if (chLen == 1 && *p < 0x20)
                {
                    tjs_char buf[20];
                    TJS_snprintf(buf, sizeof(buf) / sizeof(tjs_char), TJS_N("\\x%02x"), (int)*p);
                    hexflag = true;
                    ret += buf;
                }
                else
                {
                    ret += ttstr(p, chLen);
                    p += chLen - 1;
                    hexflag = false;
                }
        }
    }
    return ret;
}
//---------------------------------------------------------------------------
tTJSString tTJSString::UnescapeC() const
{
    // TODO: UnescapeC
    return TJS_N("");
}
//---------------------------------------------------------------------------
bool tTJSString::StartsWith(const tjs_char* string) const
{
    // return true if this starts with "string"
    if (!Ptr)
    {
        if (!*string)
            return true; // empty string starts with empty string
        return false;
    }
    const tjs_char* this_p = *Ptr;
    while (*string && *this_p)
    {
        if (*string != *this_p)
            return false;
        string++, this_p++;
    }
    if (!*string)
        return true;
    return false;
}

TJS::tTJSString tTJSString::SubString(unsigned int pos, unsigned int len) const
{
    if (Ptr == NULL || len == 0 || pos >= Ptr->GetLength())
        return tTJSString();
    if (pos == 0 && len >= Ptr->GetLength())
        return tTJSString(*this);
    return tTJSString(Ptr->operator const tjs_char*() + pos, len);
}

TJS::tTJSString tTJSString::Trim()
{
    const tjs_char* p = c_str();
    while (*p > '\0' && *p < 0x20)
        p++;

    tTJSString _str(p);
    tjs_char* p0 = (tjs_char*)_str.c_str();
    tjs_char* p1 = (tjs_char*)_str.c_str() + _str.length() - 1;
    while (p0 < p1 && *p1 != '\0' && *p1 < 0x20)
        *p1-- = '\0';
    if (_str.Ptr)
        _str.Ptr->FixLength();
    return _str;
}

int tTJSString::IndexOf(const tTJSString& str, unsigned int pos /*= 0*/) const
{
    if (!Ptr || !str.Ptr)
        return -1;
    //    if(str.length() == 0 || pos >= length()) return -1;
    const tjs_char* p =
        TJS_strstr(Ptr->operator const tjs_char*() + pos, str.Ptr->operator const tjs_char*());
    if (p == NULL)
        return -1;
    return p - Ptr->operator const tjs_char*();
}
//---------------------------------------------------------------------------
tTJSString operator+(const tjs_char* lhs, const tTJSString& rhs)
{
    tTJSString ret(lhs);
    ret += rhs;
    return ret;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
tTJSString TJSInt32ToHex(tjs_uint32 num, int zeropad)
{
    // convert given number to HEX string.
    // zeros are padded when the output string's length is smaller than "zeropad".
    // "zeropad" cannot be larger than 8.
    if (zeropad > 8)
        zeropad = 8;

    tjs_char buf[12];
    tjs_char buf2[12];

    tjs_char* p = buf;
    tjs_char* d = buf2;

    do
    {
        *(p++) = (TJS_N("0123456789ABCDEF"))[num % 16];
        num /= 16;
        zeropad--;
    } while (zeropad || num);

    p--;
    while (buf <= p)
        *(d++) = *(p--);
    *d = 0;

    return ttstr(buf2);
}
//---------------------------------------------------------------------------
} // namespace TJS
