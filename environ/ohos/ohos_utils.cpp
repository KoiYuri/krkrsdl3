// OHOS utilities — platform info, locale, time, screen, stat
// Uses POSIX/sysroot APIs

#include "tjsCommHead.h"
#include "Platform.h"
#include "PlatformFile.h"

#include <hilog/log.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstdarg>
#include <ctime>

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "krkrsdl3"

bool TVP_stat(const char* name, tTVP_stat& s)
{
    if (!name) return false;
    struct stat st;
    if (stat(name, &st) != 0) return false;
    s.tvp_isdir = S_ISDIR(st.st_mode);
    s.tvp_size = st.st_size;
    s.tvp_atime = st.st_atime;
    s.tvp_mtime = st.st_mtime;
    s.tvp_ctime = st.st_ctime;
    return true;
}

// ─── Screen — default 1280x720, updated by XComponent surface ───
int tTVPScreen::GetWidth() { return 1280; }
int tTVPScreen::GetHeight() { return 720; }
int tTVPScreen::GetDesktopLeft() { return 0; }
int tTVPScreen::GetDesktopTop() { return 0; }
int tTVPScreen::GetDesktopWidth() { return GetWidth(); }
int tTVPScreen::GetDesktopHeight() { return GetHeight(); }

void TVPConsoleLog(const tjs_char* format, ...)
{
    va_list args;
    va_start(args, format);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, args);
    OH_LOG_INFO(LOG_APP, "%{public}s", buf);
    va_end(args);
}

ttstr TVPGetPlatformName()
{
    return "OpenHarmony";
}

std::string TVPGetCurrentLanguage()
{
    // OHOS 没有标准 C locale API，返回默认
    // 可通过 i18n Kit 获取系统语言，此处保持空字符串表示默认
    return "";
}

void TVPOpenPatchLibUrl()
{
    // OHOS 无浏览器，无法直接打开 URL
}

tjs_uint64 TVPGetRoughTickCount()
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<tjs_uint64>(ts.tv_sec) * 1000 +
           static_cast<tjs_uint64>(ts.tv_nsec) / 1000000;
}
