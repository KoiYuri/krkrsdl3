#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_N("gfxEffect.dll")

static void InitPlugin_GFXEffect()
{
    TVPExecuteScript(TJS_N("class gfxFire {") TJS_N("  function gfxFire() {") TJS_N(
        "    Debug.message('gfxFire construct'); ") TJS_N("  }") TJS_N("  function finalize() {")
                         TJS_N("    Debug.message('gfxFire finalize');") TJS_N("  }") TJS_N("};"));
}

NCB_PRE_REGIST_CALLBACK(InitPlugin_GFXEffect);