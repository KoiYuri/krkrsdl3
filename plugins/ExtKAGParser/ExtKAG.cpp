#include "tjsCommHead.h"
#include "ncbind/ncbind.hpp"

#define NCB_MODULE_NAME TJS_N("ExtKAGParser.dll")
#define TVP_KAGPARSER_EX_CLASSNAME TJS_N("KAGParser")

extern tTJSNativeClass* TVPCreateNativeClass_ExtKAGParser();
static iTJSDispatch2* origKAGParser = NULL;

void extkagparser_init()
{
    iTJSDispatch2* global = TVPGetScriptDispatch();
    if (global)
    {
        tTJSVariant val;
        if (TJS_SUCCEEDED(global->PropGet(0, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global)))
        {
            origKAGParser = val.AsObject();
            val.Clear();
        }
        iTJSDispatch2* tjsclass = TVPCreateNativeClass_ExtKAGParser();
        val = tTJSVariant(tjsclass);
        tjsclass->Release();
        global->PropSet(TJS_MEMBERENSURE, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global);
        global->Release();
    }
}

void extkagparser_done()
{

    iTJSDispatch2* global = TVPGetScriptDispatch();
    if (global)
    {
        global->DeleteMember(0, TVP_KAGPARSER_EX_CLASSNAME, NULL, global);
        if (origKAGParser)
        {
            tTJSVariant val(origKAGParser);
            origKAGParser->Release();
            origKAGParser = NULL;
            global->PropSet(TJS_MEMBERENSURE, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global);
        }
        global->Release();
    }
}

NCB_PRE_REGIST_CALLBACK(extkagparser_init);
NCB_POST_UNREGIST_CALLBACK(extkagparser_done);
