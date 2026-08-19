// OHOS core — platform entry, version info, memory, resource stream
// References linux_core.cpp for implementation patterns

#include "tjsCommHead.h"
#include "Platform.h"
#include "PlatformFile.h"
#include "UtilStreams.h"
#include "TVPApplication.h"
#include "TVPMsg.h"
#include "TVPCompositor.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <hilog/log.h>
#include <rawfile/raw_file_manager.h>
#include <rawfile/raw_file.h>

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "krkrsdl3"

// ohos_entry.cpp 暴露的全局 ResourceManager
extern NativeResourceManager* OHOS_GetResourceManager();

#undef LOG_DOMAIN
#undef LOG_TAG
#define LOG_DOMAIN 0x3200
#define LOG_TAG "krkrsdl3"


//---------------------------------------------------------------------------
tTVPMemoryStream* GetResourceStream(const ttstr& filename)
{
    auto* mgr = OHOS_GetResourceManager();
    if (!mgr) {
        // 回退到文件系统方式
        ttstr path = TVPNormalizeStorageName(TVPGetDefaultFileDir()) + TJS_N("/Res/") + filename;
        tTJSBinaryStream* stream = TVPCreateBinaryStreamForRead(path, TJS_BS_READ);
        if (!stream) return nullptr;
        tTVPMemoryStream* ret = new tTVPMemoryStream(nullptr, stream->GetSize());
        stream->ReadBuffer(ret->GetInternalBuffer(), stream->GetSize());
        delete stream;
        return ret;
    }

    // 使用 OHOS RawFile API 从 rawfile 读取
    std::string rawPath = "Res/" + filename.AsStdString();
    RawFile* rawFile = OH_ResourceManager_OpenRawFile(mgr, rawPath.c_str());
    if (!rawFile) {
        OH_LOG_WARN(LOG_APP, "RawFile not found: %{public}s", rawPath.c_str());
        return nullptr;
    }

    long size = OH_ResourceManager_GetRawFileSize(rawFile);
    if (size <= 0) {
        OH_ResourceManager_CloseRawFile(rawFile);
        return nullptr;
    }

    tTVPMemoryStream* ret = new tTVPMemoryStream(nullptr, size);
    int bytesRead = OH_ResourceManager_ReadRawFile(rawFile, ret->GetInternalBuffer(), size);
    OH_ResourceManager_CloseRawFile(rawFile);

    if (bytesRead != size) {
        delete ret;
        return nullptr;
    }
    return ret;
}

//---------------------------------------------------------------------------
std::string TVPGetPackageVersionString()
{
    return "ohos";
}

ttstr TVPGetOSName()
{
    // 读取 OHOS 系统版本
    std::string fullname;
    std::ifstream file("/etc/ohos-release");
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            if (line.find("OpenHarmony") != std::string::npos ||
                line.find("HarmonyOS") != std::string::npos) {
                fullname = line;
                break;
            }
        }
    }
    if (!fullname.empty()) return fullname;

    // 尝试 sys_properties
    std::ifstream prop("/proc/version");
    if (prop.is_open()) {
        std::string line;
        std::getline(prop, line);
        if (!line.empty()) return "OpenHarmony (" + line.substr(0, 50) + ")";
    }
    return "OpenHarmony";
}

//---------------------------------------------------------------------------
void TVPInvokeMenu(int x, int y, void* _menu)
{
    // OHOS 菜单通过 ArkTS 实现，C++ 侧无需处理
}

// ─── Software renderer texture backend (stub — OHOS uses GL) ──────
void TVPCreateTextureBackend(TVPSprite&) {}
void TVPRenderTextureBackend(TVPSprite*, int, int, int, int) {}
void TVPUpdateTextureBackend(TVPSprite*, uint8_t*, int, int, int) {}
void TVPDestroyTextureBackend(TVPSprite*) {}

// ─── libpng NEON stubs (compiled without ARM NEON intrinsics) ─────
extern "C" {
void png_riffle_palette_neon(void*, void*, int, int) {}
void png_do_expand_palette_rgba8_neon(void*, void*, int, int, int) {}
void png_do_expand_palette_rgb8_neon(void*, void*, int, int, int) {}
void png_init_filter_functions_neon(void*, unsigned int) {}
}
