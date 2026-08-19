#pragma once

#include "tvpinputdefs.h"

namespace krkrsdl3
{
// 事件传递
void KRKR_Trig_MouseDown(tTVPMouseButton mouseId, int x, int y);
void KRKR_Trig_MouseUp(tTVPMouseButton mouseId, int x, int y);
void KRKR_Trig_MouseMove(int x, int y);
void KRKR_Trig_MouseScroll(int dx, int dy, int x, int y);
void KRKR_Trig_KeyDown(int vk);
void KRKR_Trig_KeyUp(int vk);
void KRKR_Trig_TextInput(std::string text);
} // namespace krkrsdl3