#pragma once
#include <cstdint>
#include <vector>

#include "PlatformView.h"

//---------------------------------------------------------------------------
// Windows Compositor
//---------------------------------------------------------------------------
namespace krkrsdl3
{
// 
void fetchGLInfo();
TVPSprite* KRKR_Get_Current_Sprite();
// 贴图管理
void TVPJoinTexture(TVPSprite* sp);
void TVPDepartTexture(TVPSprite* sp);
// 渲染函数
void TVPRenderOnce(int winWidth, int winHeight);
void TVPCreateTexture(TVPSprite& sp);
void TVPUpdateTexture(TVPSprite* sp, uint8_t* buff, int width, int height, int pitch);
void TVPDestroyTexture(TVPSprite* sp);
// 清理合成器中所有窗口贴图
void TVPClearAllTexture();
} // namespace krkrsdl3