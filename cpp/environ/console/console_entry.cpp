#include <stdio.h>

#include "tjsCommHead.h"
#include "tjs.h"
#include "tjsDebug.h"
#include "tjsArray.h"
#include "tjsRandomGenerator.h"

#include "console_init.cpp"

tTJS* TVPScriptEngine;
ttstr TVPStartupScriptName(TJS_N("startup.tjs"));

int main(int argc, char* argv[])
{
    /*
    * 初始化
    */
    TVPScriptEngine = new tTJS();

    TJSGetRandomBits128 = TVPGetRandomBits128;

    // script system initialization
    TVPScriptEngine->ExecScript(ttstr(TVPInitTJSScript));

    // set console output gateway handler
    TVPScriptEngine->SetConsoleOutput(&TVPTJS2ConsoleOutputGateway);

    // set text stream functions
    TJSCreateTextStreamForRead = TVPCreateTextStreamForRead;
    TJSCreateTextStreamForWrite = TVPCreateTextStreamForWrite;

    // set binary stream functions
    TJSCreateBinaryStreamForRead = TVPCreateBinaryStreamForRead;
    TJSCreateBinaryStreamForWrite = TVPCreateBinaryStreamForWrite;

    /*
     * 注册函数
     */
    // register some TVP classes/objects/functions/propeties
    iTJSDispatch2* dsp;
    iTJSDispatch2* global = TVPScriptEngine->GetGlobalNoAddRef();
    tTJSVariant val;

#define REGISTER_OBJECT(classname, instance) \
    dsp = (instance); \
    val = tTJSVariant(dsp /*, dsp*/); \
    dsp->Release(); \
    global->PropSet(TJS_MEMBERENSURE | TJS_IGNOREPROP, TJS_N(#classname), NULL, &val, global);

    REGISTER_OBJECT(Debug, TVPCreateNativeClass_Debug());

    /*
     * 读取脚本
     */
    FILE* file = fopen(TVPStartupScriptName.c_str(), "rb");
    if (!file)
    {
        printf("ERROR: Cannot open %s\n", TVPStartupScriptName.c_str());
        return 1;
    }
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    char* buffer = (char*)malloc(size + 1);
    fread(buffer, 1, size, file);
    buffer[size] = '\0';
    fclose(file);
    ttstr script_text(buffer);
    free(buffer);

    /*
     * 执行脚本
     */
    ttstr scriptError;
    try
    {
        tTJSVariant result;
        TVPScriptEngine->ExecScript(script_text, &result);
        delete TVPScriptEngine;
    }
    catch (const TJS::eTJSScriptError& e)
    {
        ttstr& msg = scriptError;
        msg += e.GetMessage();
        const tjs_char* pszBlockName = e.GetBlockName();
        if (pszBlockName && *pszBlockName)
        {
            msg += TJS_N("\n@line(");
            tjs_char tmp[34];
            msg += TJS_int_to_str(e.GetSourceLine(), tmp);
            msg += TJS_N(") ");
            msg += pszBlockName;
        }
        msg += TJS_N("\n");
        msg += e.GetTrace();
        printf(msg.c_str());
        return 1;
    }
    catch (const TJS::eTJS& e)
    {
        scriptError = e.GetMessage();
        printf(scriptError.c_str());
        return 1;
    }
    catch (const std::exception& e)
    {
        scriptError = e.what();
        printf(scriptError.c_str());
        return 1;
    }
    catch (const tjs_char* e)
    {
        scriptError = e;
        printf(scriptError.c_str());
        return 1;
    }
    return 0;
}