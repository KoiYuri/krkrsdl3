#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_N("motionplayer.dll")

static void InitPlugin_MotionPlayer()
{
    ncbAutoRegister::LoadModule(TJS_N("emoteplayer.dll"));
}

NCB_PRE_REGIST_CALLBACK(InitPlugin_MotionPlayer);