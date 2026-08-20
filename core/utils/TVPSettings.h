#pragma once

#include <string>


//---------------------------------------------------------------------------
// Command line parameter operations (implement in each platform)
//---------------------------------------------------------------------------
bool TVPParseArguments(int argc, char* argv[]);
//
void TVPClearAllArguments();
//
bool TVPGetCommandLine(const tjs_char* name, tTJSVariant* value = 0);
// retrieves command line parameter named "name".
// command line parameter format must be "-name=value"
// returns false if the the parameter is not exist, otherwise
// sets the value to "value" and returns true.
void TVPSetCommandLine(const tjs_char* name, const ttstr& value);
// sets command line to the specified value.
// note that this function does not check any consistency or correctness of the value.
// 
// 全局配置
struct TVPGlobalSettings
{
    bool ogl_accurate_render;
    std::string default_font;
    bool force_default_font;
    std::string renderer; // 解析后的渲染后端名：software / opengl / vulkan（见 core/render/backend）
    int window_width;     // 初始窗口宽度（-window=WxH，默认 1280）
    int window_height;    // 初始窗口高度（-window=WxH，默认 720）
    int vsync;            // 垂直同步开关 0/1（-vsync=0/1，默认 1）
    int software_draw_thread;
};
extern TVPGlobalSettings TVPSettings;