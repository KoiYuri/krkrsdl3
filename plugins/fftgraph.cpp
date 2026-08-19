#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_N("fftgraph.dll")

static void InitPlugin()
{
    TVPExecuteScript(TJS_N("function drawFFTGraph(){}"));
}

NCB_PRE_REGIST_CALLBACK(InitPlugin);
