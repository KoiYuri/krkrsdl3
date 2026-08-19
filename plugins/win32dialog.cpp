
#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_N("win32dialog.dll")

static void InitPlugin_WIN32Dialog()
{
    TVPExecuteScript(TJS_N("class WIN32Dialog { 	function messageBox(message, caption, "
                           "type) {return !System.inform(message, caption, 2);} }"));
}

NCB_PRE_REGIST_CALLBACK(InitPlugin_WIN32Dialog);
