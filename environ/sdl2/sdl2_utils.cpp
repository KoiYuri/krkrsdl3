#include "tjsCommHead.h"

#include "Platform.h"
#include "PlatformFile.h"

#include <SDL.h>

#ifdef _WIN32
#include <windows.h>
#endif

bool TVP_stat(const char* name, tTVP_stat& s)
{
    if (!name)
        return false;

    SDL_RWops* rw = SDL_RWFromFile(name, "rb");
    if (!rw)
    {
        return false;
    }

    Sint64 fsize = SDL_RWsize(rw);
    SDL_RWclose(rw);

    s.tvp_isdir = false;
    s.tvp_size = (tjs_uint64)fsize;
    s.tvp_atime = 0;
    s.tvp_mtime = 0;
    s.tvp_ctime = 0;

    return true;
}

int tTVPScreen::GetWidth()
{
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0)
        return mode.w;
    return 0;
}

int tTVPScreen::GetHeight()
{
    SDL_DisplayMode mode;
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0)
        return mode.h;
    return 0;
}

int tTVPScreen::GetDesktopLeft()
{
    SDL_Rect rect;
    if (SDL_GetDisplayBounds(0, &rect) == 0)
        return 0;
    return 0;
}

int tTVPScreen::GetDesktopTop()
{
    SDL_Rect rect;
    if (SDL_GetDisplayBounds(0, &rect) == 0)
        return 0;
    return 0;
}

int tTVPScreen::GetDesktopWidth()
{
    SDL_Rect rect;
    if (SDL_GetDisplayBounds(0, &rect) == 0)
        return rect.w;
    return GetWidth();
}

int tTVPScreen::GetDesktopHeight()
{
    SDL_Rect rect;
    if (SDL_GetDisplayBounds(0, &rect) == 0)
        return rect.h;
    return GetHeight();
}

void TVPConsoleLog(const tjs_char* format, ...)
{
    va_list args;
    va_start(args, format);
    SDL_LogMessageV(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, format, args);
    va_end(args);
}

ttstr TVPGetPlatformName()
{
    const char* platform = SDL_GetPlatform();
    return ttstr(platform);
}

std::string TVPGetCurrentLanguage()
{
    std::string ret = "";
#ifdef _WIN32
    char lang[64] = {0};
    if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, lang, sizeof(lang)))
    {
        ret = lang;
        char country[64] = {0};
        if (GetLocaleInfoA(LOCALE_USER_DEFAULT, LOCALE_SISO3166CTRYNAME, country, sizeof(country)))
        {
            ret += std::string("_") + country;
        }
    }
#endif
    return ret;
}

void TVPOpenPatchLibUrl()
{
    std::string url = "https://zeas2.github.io/Kirikiroid2_patch/patch";
    SDL_OpenURL(url.c_str());
}

tjs_uint64 TVPGetRoughTickCount()
{
    return SDL_GetTicks();
}
