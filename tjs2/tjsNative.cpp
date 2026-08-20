//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Support for C++ native class/method/property definitions
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsNative.h"
#include "tjsError.h"
#include "tjsGlobalStringMap.h"
#include "tjsDebug.h"

namespace TJS
{
//---------------------------------------------------------------------------
static std::vector<ttstr> NativeClassNames;
static bool _tjsForceShutdown = false;
static tTJSObjectPool<tTJSDispatch> _allTJSRegister;
static std::vector<std::pair<_TJSStaticClearFun*, void*>> _allTJSStaticClearFun;
void TJSCompactRegisterHeap()
{
    // TODO
}
void TJSClearRegisterHeap()
{
    _tjsForceShutdown = true;

    _allTJSRegister.forEach([](tTJSDispatch* obj) { delete obj; });
    _allTJSRegister.clear();
    NativeClassNames.clear();

    for (auto obj : _allTJSStaticClearFun)
    {
        obj.first(obj.second);
    }
    _allTJSStaticClearFun.clear();

    _tjsForceShutdown = false;
}
void TJSAddStaticToRegisterHeap(_TJSStaticClearFun* clsFun, void* ptr)
{
    _allTJSStaticClearFun.push_back(std::make_pair(clsFun, ptr));
}

//---------------------------------------------------------------------------
// NativeClass registration
//---------------------------------------------------------------------------
tjs_int32 TJSRegisterNativeClass(const tjs_char* name)
{
    for (tjs_uint i = 0; i < NativeClassNames.size(); i++)
    {
        if (NativeClassNames[i] == name)
            return i;
    }

    NativeClassNames.push_back(TJSMapGlobalStringMap(name));

    return (tjs_int32)(NativeClassNames.size() - 1);
}
//---------------------------------------------------------------------------
tjs_int32 TJSFindNativeClassID(const tjs_char* name)
{
    for (tjs_uint i = 0; i < NativeClassNames.size(); i++)
    {
        if (NativeClassNames[i] == name)
            return i;
    }

    return -1;
}
//---------------------------------------------------------------------------
const tjs_char* TJSFindNativeClassName(tjs_int32 id)
{
    if (id < 0 || id >= (tjs_int32)NativeClassNames.size())
        return NULL;
    return NativeClassNames[id].c_str();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNativeClassMethod
//---------------------------------------------------------------------------
tTJSNativeClassMethod::tTJSNativeClassMethod(tTJSNativeClassMethodCallback processfunc)
{
    Process = processfunc;
    if (TJSObjectHashMapEnabled())
        TJSAddObjectHashRecord(this);
    this->GCIndex = _allTJSRegister.registerObject(this);
}
//---------------------------------------------------------------------------
tTJSNativeClassMethod::~tTJSNativeClassMethod()
{
    if (TJSObjectHashMapEnabled())
        TJSRemoveObjectHashRecord(this);
    if (!_tjsForceShutdown)
    {
        _allTJSRegister.unregisterObject(this->GCIndex);
    }
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClassMethod::IsInstanceOf(tjs_uint32 flag,
                                              const tjs_char* membername,
                                              tjs_uint32* hint,
                                              const tjs_char* classname,
                                              iTJSDispatch2* objthis)
{
    if (membername == NULL)
    {
        if (!TJS_strcmp(classname, TJS_N("Function")))
            return TJS_S_TRUE;
    }

    return inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClassMethod::FuncCall(tjs_uint32 flag,
                                          const tjs_char* membername,
                                          tjs_uint32* hint,
                                          tTJSVariant* result,
                                          tjs_int numparams,
                                          tTJSVariant** param,
                                          iTJSDispatch2* objthis)
{
    if (membername)
        return inherited::FuncCall(flag, membername, hint, result, numparams, param, objthis);
    if (!objthis)
        return TJS_E_NATIVECLASSCRASH;

    if (result)
        result->Clear();
    tjs_error er;
    try
    {
        er = Process(result, numparams, param, objthis);
    }
    catch (...)
    {
        throw;
    }
    return er;
}
//---------------------------------------------------------------------------
tTJSNativeClassMethod* TJSCreateNativeClassMethod(tTJSNativeClassMethodCallback callback)
{
    return new tTJSNativeClassMethod(callback);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNativeClassConstructor
//---------------------------------------------------------------------------
tjs_error tTJSNativeClassConstructor::FuncCall(tjs_uint32 flag,
                                               const tjs_char* membername,
                                               tjs_uint32* hint,
                                               tTJSVariant* result,
                                               tjs_int numparams,
                                               tTJSVariant** param,
                                               iTJSDispatch2* objthis)
{
    if (membername)
        return tTJSDispatch::FuncCall(flag, membername, hint, result, numparams, param, objthis);
    if (result)
        result->Clear();
    tjs_error er;
    try
    {
        er = Process(result, numparams, param, objthis);
    }
    catch (...)
    {
        throw;
    }
    return er;
}
//---------------------------------------------------------------------------
tTJSNativeClassMethod* TJSCreateNativeClassConstructor(tTJSNativeClassMethodCallback callback)
{
    return new tTJSNativeClassConstructor(callback);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNativeClassProperty
//---------------------------------------------------------------------------
tTJSNativeClassProperty::tTJSNativeClassProperty(tTJSNativeClassPropertyGetCallback get,
                                                 tTJSNativeClassPropertySetCallback set)
{
    Get = get;
    Set = set;
    if (TJSObjectHashMapEnabled())
        TJSAddObjectHashRecord(this);
    this->GCIndex = _allTJSRegister.registerObject(this);
}
//---------------------------------------------------------------------------
tTJSNativeClassProperty::~tTJSNativeClassProperty()
{
    if (TJSObjectHashMapEnabled())
        TJSRemoveObjectHashRecord(this);
    if (!_tjsForceShutdown)
    {
        _allTJSRegister.unregisterObject(this->GCIndex);
    }
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClassProperty::IsInstanceOf(tjs_uint32 flag,
                                                const tjs_char* membername,
                                                tjs_uint32* hint,
                                                const tjs_char* classname,
                                                iTJSDispatch2* objthis)
{
    if (membername == NULL)
    {
        if (!TJS_strcmp(classname, TJS_N("Property")))
            return TJS_S_TRUE;
    }

    return inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClassProperty::PropGet(tjs_uint32 flag,
                                           const tjs_char* membername,
                                           tjs_uint32* hint,
                                           tTJSVariant* result,
                                           iTJSDispatch2* objthis)
{
    if (membername)
        return inherited::PropGet(flag, membername, hint, result, objthis);
    if (!objthis)
        return TJS_E_NATIVECLASSCRASH;

    if (!result)
        return TJS_E_FAIL;

    tjs_error er;
    try
    {
        er = Get(result, objthis);
    }
    catch (...)
    {
        throw;
    }

    return er;
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClassProperty::PropSet(tjs_uint32 flag,
                                           const tjs_char* membername,
                                           tjs_uint32* hint,
                                           const tTJSVariant* param,
                                           iTJSDispatch2* objthis)
{
    if (membername)
        return inherited::PropSet(flag, membername, hint, param, objthis);
    if (!objthis)
        return TJS_E_NATIVECLASSCRASH;

    if (!param)
        return TJS_E_FAIL;

    tjs_error er;
    try
    {
        er = Set(param, objthis);
    }
    catch (...)
    {
        throw;
    }

    return er;
}
//---------------------------------------------------------------------------
tTJSNativeClassProperty* TJSCreateNativeClassProperty(tTJSNativeClassPropertyGetCallback get,
                                                      tTJSNativeClassPropertySetCallback set)
{
    return new tTJSNativeClassProperty(get, set);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNativeClass
//---------------------------------------------------------------------------
tTJSNativeClass::tTJSNativeClass(const ttstr& name)
{
    CallFinalize = false;
    ClassName = TJSMapGlobalStringMap(name);

    if (TJSObjectHashMapEnabled())
        TJSObjectHashSetType(this, ttstr(TJS_N("(native class) ")) + ClassName);
}
//---------------------------------------------------------------------------
tTJSNativeClass::~tTJSNativeClass()
{
}
//---------------------------------------------------------------------------
void tTJSNativeClass::RegisterNCM(const tjs_char* name,
                                  iTJSDispatch2* dsp,
                                  const tjs_char* classname,
                                  tTJSNativeInstanceType type,
                                  tjs_uint32 flags)
{
    // map name via Global String Map
    ttstr tname = TJSMapGlobalStringMap(ttstr(name));

    // set object type for debugging
    if (TJSObjectHashMapEnabled())
    {
        switch (type)
        {
            case nitMethod:
                TJSObjectHashSetType(dsp, ttstr(TJS_N("(native function) ")) + classname +
                                              TJS_N(".") + name);
                break;
            case nitProperty:
                TJSObjectHashSetType(dsp, ttstr(TJS_N("(native property) ")) + classname +
                                              TJS_N(".") + name);
                break;
                /*
                case nitClass:
                        The information is not set here
                        (is to be set in tTJSNativeClass::tTJSNativeClass)
                */
            default:
                break;
        }
    }

    // add to this
    tTJSVariant val;
    val = dsp;
    if (PropSetByVS((TJS_MEMBERENSURE | TJS_IGNOREPROP) | flags, tname.AsVariantStringNoAddRef(),
                    &val, this) == TJS_E_NOTIMPL)
        PropSet((TJS_MEMBERENSURE | TJS_IGNOREPROP) | flags, tname.c_str(), NULL, &val, this);

    // release dsp
    dsp->Release();
}
//---------------------------------------------------------------------------
void tTJSNativeClass::Finalize(void)
{
    tTJSCustomObject::Finalize();
}
//---------------------------------------------------------------------------
iTJSDispatch2* tTJSNativeClass::CreateBaseTJSObject()
{
    if (SuperClass)
    {
        return new tTJSExtendableObject;
    }
    else
    {
        return new tTJSCustomObject;
    }
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClass::FuncCall(tjs_uint32 flag,
                                    const tjs_char* membername,
                                    tjs_uint32* hint,
                                    tTJSVariant* result,
                                    tjs_int numparams,
                                    tTJSVariant** param,
                                    iTJSDispatch2* objthis)
{
    if (!GetValidity())
        return TJS_E_INVALIDOBJECT;

    if (membername)
    {
        tjs_error hr =
            tTJSCustomObject::FuncCall(flag, membername, hint, result, numparams, param, objthis);
        if (hr == TJS_E_MEMBERNOTFOUND && SuperClass != NULL)
        {
            hr = SuperClass->FuncCall(flag, membername, hint, result, numparams, param, objthis);
        }
        return hr;
    }

    tTJSVariant name(ClassName);
    objthis->ClassInstanceInfo(TJS_CII_ADD, 0, &name); // add class name

    // create base native object
    iTJSNativeInstance* nativeptr = CreateNativeInstance();

    // register native instance information to the object;
    // even if "nativeptr" is null
    objthis->NativeInstanceSupport(TJS_NIS_REGISTER, _ClassID, &nativeptr);

    // register members to "objthis"

    // a class to receive member callback from class
    class tCallback : public tTJSDispatch
    {
    public:
        iTJSDispatch2* Dest; // destination object
        tjs_error FuncCall(tjs_uint32 flag,
                           const tjs_char* membername,
                           tjs_uint32* hint,
                           tTJSVariant* result,
                           tjs_int numparams,
                           tTJSVariant** param,
                           iTJSDispatch2* objthis)
        {
            // *param[0] = name   *param[1] = flags   *param[2] = value
            tjs_uint32 flags = (tjs_int)*param[1];
            if (!(flags & TJS_STATICMEMBER))
            {
                tTJSVariant val = *param[2];
                if (val.Type() == tvtObject)
                {
                    // change object's objthis if the object's objthis is null
                    if (val.AsObjectThisNoAddRef() == NULL)
                        val.ChangeClosureObjThis(Dest);
                }

                if (Dest->PropSetByVS(TJS_MEMBERENSURE | TJS_IGNOREPROP | flags,
                                      param[0]->AsStringNoAddRef(), &val, Dest) == TJS_E_NOTIMPL)
                    Dest->PropSet(TJS_MEMBERENSURE | TJS_IGNOREPROP | flags, param[0]->GetString(),
                                  NULL, &val, Dest);
            }
            if (result)
                *result = (tjs_int)(1); // returns true
            return TJS_S_OK;
        }
    };

    tCallback callback;
    callback.Dest = objthis;

    // enumerate members
    tTJSVariantClosure clo(&callback, (iTJSDispatch2*)NULL);
    EnumMembers(TJS_IGNOREPROP, &clo, this);

    return TJS_S_OK;
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClass::CreateNew(tjs_uint32 flag,
                                     const tjs_char* membername,
                                     tjs_uint32* hint,
                                     iTJSDispatch2** result,
                                     tjs_int numparams,
                                     tTJSVariant** param,
                                     iTJSDispatch2* objthis)
{
    // CreateNew
    iTJSDispatch2* superinst = NULL;
    if (SuperClass != NULL && membername == NULL)
    {
        tjs_error hr =
            SuperClass->CreateNew(flag, membername, hint, &superinst, numparams, param, objthis);
        if (TJS_FAILED(hr))
        {
            superinst = NULL;
        }
    }

    iTJSDispatch2* dsp = CreateBaseTJSObject();

    tjs_error hr;
    try
    {
        // set object type for debugging
        if (TJSObjectHashMapEnabled())
            TJSObjectHashSetType(dsp, TJS_N("instance of class ") + ClassName);

        // instance initialization
        hr = FuncCall(0, NULL, NULL, NULL, 0, NULL, dsp); // add member to dsp

        if (TJS_FAILED(hr))
            return hr;

        if (superinst != NULL)
        {
            tTJSVariant param(superinst, superinst);
            dsp->ClassInstanceInfo(TJS_CII_SET_SUPRECLASS, 0, &param);
            superinst->Release();
            superinst = NULL;
        }

        hr = FuncCall(0, ClassName.c_str(), ClassName.GetHint(), NULL, numparams, param, dsp);
        // call the constructor
        if (hr == TJS_E_MEMBERNOTFOUND)
            hr = TJS_S_OK;
        // missing constructor is OK ( is this ugly ? )
    }
    catch (...)
    {
        dsp->Release();
        if (superinst)
            superinst->Release();
        throw;
    }

    if (TJS_SUCCEEDED(hr))
    {
        *result = dsp;
    }
    else if (superinst)
    {
        superinst->Release();
    }
    return hr;
}
//---------------------------------------------------------------------------
tjs_error tTJSNativeClass::IsInstanceOf(tjs_uint32 flag,
                                        const tjs_char* membername,
                                        tjs_uint32* hint,
                                        const tjs_char* classname,
                                        iTJSDispatch2* objthis)
{
    if (membername == NULL)
    {
        if (!TJS_strcmp(classname, TJS_N("Class")))
            return TJS_S_TRUE;
        if (!TJS_strcmp(classname, ClassName.c_str()))
            return TJS_S_TRUE;
    }

    return inherited::IsInstanceOf(flag, membername, hint, classname, objthis);
}
//---------------------------------------------------------------------------
} // namespace TJS
