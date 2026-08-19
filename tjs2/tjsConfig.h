//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// configuration
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

#pragma once

#include <time.h>
#include <cstdarg>

//---------------------------------------------------------------------------
/*
        many settings can be changed here.

        tjsCommHead.h includes most common headers that will be needed to
        compile the entire TJS program.

        configuration about Critical Section for multithreading support is there in
        tjsUtils.cpp/h.
*/

// TODO: autoconf integration

#include "tjsTypes.h"

namespace TJS
{
#define TJS_nsprintf sprintf
#define TJS_nstrcpy strcpy
#define TJS_nstrcat strcat
#define TJS_nstrlen strlen
#define TJS_nstrstr strstr
#define TJS_vfprintf vfwprintf
#define TJS_octetcpy memcpy
#define TJS_octetcmp memcmp

tjs_int TJS_atoi(const tjs_char* s);
tjs_char* TJS_int_to_str(tjs_int value, tjs_char* string);
tjs_char* TJS_tTVInt_to_str(tjs_int64 value, tjs_char* string);
tjs_int TJS_strnicmp(const tjs_char* s1, const tjs_char* s2, size_t maxlen);
tjs_int TJS_stricmp(const tjs_char* s1, const tjs_char* s2);
void TJS_strcpy_maxlen(tjs_char* d, const tjs_char* s, size_t len);
void TJS_strcpy(tjs_char* d, const tjs_char* s);
size_t TJS_strlen(const tjs_char* d);
tjs_int64 TJS_atoll(const tjs_char* s);
tjs_int TJS_sprintf(tjs_char* s, const tjs_char* format, ...);
tjs_int TJS_timezone();

#define TJS_strncpy_s(d, dl, s, sl) TJS_strncpy(d, s, sl)

#define TJS_narrowtowidelen(X) TJS_mbstowcs(NULL, (X), 0) // narrow->wide (if) converted length
#define TJS_narrowtowide TJS_mbstowcs

void TJS_debug_out(const tjs_char* format, ...);

#if TJS_DEBUG_TRACE
#define TJS_D(x) TJS_debug_out x;
#define TJS_F_TRACE(x) tTJSFuncTrace ___trace(TJS_N(x));
#else
#define TJS_D(x)
#define TJS_F_TRACE(x)
#endif

size_t TJS_mbstowcs(tjs_wchar* pwcs, const tjs_char* s, size_t n);
size_t TJS_wcstombs(tjs_char* s, const tjs_wchar* pwcs, size_t n);
int TJS_mbtowc(tjs_wchar* pwc, const tjs_char* s, size_t n);
int TJS_wctomb(tjs_char* s, tjs_wchar wc);
tjs_int64 TVPWideCharToUtf8(tjs_wchar in, char* out);
tjs_int64 TVPWideCharToUtf8String(const tjs_wchar* in, char* out, size_t max_len = 0);
bool TVP_utf8_to_utf16(const char* in, tjs_wchar* out);
tjs_int64 TVPUtf8ToWideCharString(const char* in, tjs_wchar* out);
tjs_int64 TVPUtf8ToWideCharString(const char* in, tjs_uint length, tjs_wchar* out);
uint16_t utf8_to_unicode(const char* utf8);
std::vector<uint32_t> decodeUTF8ToTTF(const char* utf8_str);
std::vector<tjs_wchar> decodeUTF8ToTTFe(const char* utf8_str);
tjs_int64 utf8_char_len(const char* str);
tjs_int64 utf8_chars_len(const char* str);
tjs_int64 utf8_indexOf(const char* data, const char* chs, int startpos = 0);
const char* utf8_substr(const char* data, int start, int len, int& retLen);
void utf8_reverse(const char* data, char* outputdata);
const char* utf8_char_get(const char* str, int idx);
bool TJS_iswspace(const char* chs);
bool TJS_iswdigit(const char* chs);
tjs_int TJS_iswalpha(const char* chs);

int TJS_strcmp(const tjs_char* src, const tjs_char* dst);
int TJS_strncmp(const tjs_char* first, const tjs_char* last, size_t count);
int TJS_strcasecmp(const tjs_char* s1, const tjs_char* s2);
int TJS_strncasecmp(const tjs_char* s1, const tjs_char* s2, size_t n);
tjs_char* TJS_strncpy(tjs_char* dest, const tjs_char* source, size_t count);
tjs_char* TJS_strcat(tjs_char* dst, const tjs_char* src);
tjs_char* TJS_strstr(const tjs_char* wcs1, const tjs_char* wcs2);
tjs_char* TJS_strchr(const tjs_char* string, tjs_char ch);
void* TJS_malloc(size_t n);
void* TJS_realloc(void* orig, size_t n);
void TJS_free(void* p);
tjs_char* TJS_strrchr(const tjs_char* s, int c);

double TJS_strtod(const tjs_char* pString, tjs_char** ppEnd);
size_t TJS_strftime(tjs_char* wstring, size_t maxsize, const tjs_char* wformat, const tm* timeptr);
int TJS_vsnprintf(tjs_char* string, size_t count, const tjs_char* format, va_list ap);
tjs_int TJS_snprintf(tjs_char* s, size_t count, const tjs_char* format, ...);

void TJSNativeDebuggerBreak();
void TJSSetFPUE();
void TJSRestoreFPUE();

//---------------------------------------------------------------------------
// elapsed time profiler
//---------------------------------------------------------------------------
#if TJS_DEBUG_PROFILE_TIME
extern tjs_uint TJSGetTickCount();
class tTJSTimeProfiler
{
    tjs_uint& timevar;
    tjs_uint start;

public:
    tTJSTimeProfiler(tjs_uint& tv) : timevar(tv) { start = TJSGetTickCount(); }

    ~tTJSTimeProfiler() { timevar += TJSGetTickCount() - start; }
};
#endif
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// function tracer
//---------------------------------------------------------------------------
class tTJSFuncTrace
{
    tjs_char* funcname;

public:
    tTJSFuncTrace(tjs_char* p)
    {
        funcname = p;
        TJS_debug_out(TJS_N("enter: %s\n"), funcname);
    }
    ~tTJSFuncTrace() { TJS_debug_out(TJS_N("exit: %s\n"), funcname); }
};
//---------------------------------------------------------------------------
}; // namespace TJS
