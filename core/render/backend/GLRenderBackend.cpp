#include "GLRenderBackend.h"

#include "tjsCommHead.h"

#ifdef _KRKRSDL3_GL
#include "glad/glad.h"
#else
#include <GLES3/gl3.h>
#endif

#include <algorithm>
#include <cstring>
#include <string>
#include <unordered_set>
#include <vector>

#include "Platform.h"
#include "PlatformView.h"
#include "TVPDebug.h"

//---------------------------------------------------------------------------
// OpenGL / OpenGL ES 渲染后端（合并实现）
//
// 一个类同时承担两个角色（同一接口内，见 RenderBackend.h）：
//   1. 窗口贴图合成后端——由 cpp/core/render/TVPCompositor.cpp
//      的 GL 路径迁移而来（同样的 shader、同样的顶点数据、同样的纹理参数）。
//   2. 2D 网格渲染器——由 cpp/plugins/emoteplayer/emoterunner.cpp
//      的 GL 渲染路径迁移而来（同一套 shader、蒙版采样语义、bm 混合映射）。
//
// GPU 后端下窗口贴图与一般贴图就是同一套 GL 纹理函数
// （CreateWindowTexture == CreateTexture 等），因此合并为一个类维护。
//---------------------------------------------------------------------------
namespace krkrsdl3
{
static std::unordered_set<std::string> sTVPGLExtensions;

//---------------------------------------------------------------------------
// 窗口合成 shader（330 core / 300 es）
//---------------------------------------------------------------------------
#if defined(_KRKRSDL3_GL) && _KRKRSDL3_GL
const char* kWindowVertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

uniform vec2 windowSize;
uniform vec2 texture_Position;
uniform vec2 texture_Size;

void main()
{
    vec2 pixelPos = texture_Position + vec2(texture_Size.x * aPos.x, texture_Size.y * aPos.y);
    vec2 ndcPos = pixelPos * 2.0 - 1.0;

    gl_Position = vec4(ndcPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";
const char* kWindowFragmentShaderSrc = R"(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
}
)";
#else
const char* kWindowVertexShaderSrc = R"(#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
out vec2 TexCoord;

uniform vec2 windowSize;
uniform vec2 texture_Position;
uniform vec2 texture_Size;

void main()
{
    vec2 pixelPos = texture_Position + vec2(texture_Size.x * aPos.x, texture_Size.y * aPos.y);
    vec2 ndcPos = pixelPos * 2.0 - 1.0;

    gl_Position = vec4(ndcPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";
const char* kWindowFragmentShaderSrc = R"(#version 300 es
precision mediump float;
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D texture1;

void main()
{
    FragColor = texture(texture1, TexCoord);
}
)";
#endif

//---------------------------------------------------------------------------
// 2D 网格 shader（330 core / 100 es）
//---------------------------------------------------------------------------
#if defined(_KRKRSDL3_GL) && _KRKRSDL3_GL
const char* kMeshVertexShaderSrc = R"(
            #version 330 core
            layout (location = 0) in vec2 aPos;
            layout (location = 1) in vec2 aTexCoord;
            out vec2 texCoord;

            void main()
            {
                gl_Position = vec4(aPos.xy, 0.0, 1.0);
                texCoord = aTexCoord;
            }
            )";
const char* kMeshFragmentShaderSrc = R"(
            #version 330 core
            out vec4 FragColor;
            in vec2 texCoord;
            uniform sampler2D texture1;
            uniform bool enableMask;
            uniform vec2 viewportSize;
            uniform sampler2D maskTexture;
            uniform float opa;
            uniform bool enableColor;
            uniform vec4 uniformColor;
            void main()
            {
                vec4 maskColor = vec4(1.0f);
                if (enableMask) {
                    vec2 normalizedCoord = gl_FragCoord.xy / viewportSize;
                    maskColor = texture(maskTexture, normalizedCoord);
                }

                vec4 color = texture(texture1, texCoord);
                if (enableMask && maskColor.a < 0.5) {
                    discard;
                } else {
                    if(enableColor)
                    {
                        color = vec4(uniformColor.xyz, uniformColor.a * color.a);
                    }
                    color.a = color.a * opa;
                    FragColor = vec4(color.rgba);
                }
            }
        )";
#else
const char* kMeshVertexShaderSrc = R"(#version 100
            attribute vec2 aPos;
            attribute vec2 aTexCoord;
            varying vec2 texCoord;

            void main()
            {
                gl_Position = vec4(aPos.xy, 0.0, 1.0);
                texCoord = aTexCoord;
            }
            )";
const char* kMeshFragmentShaderSrc = R"(#version 100
            precision mediump float;
            varying vec2 texCoord;
            uniform sampler2D texture1;
            uniform bool enableMask;
            uniform vec2 viewportSize;
            uniform sampler2D maskTexture;
            uniform float opa;
            uniform bool enableColor;
            uniform vec4 uniformColor;
            void main()
            {
                vec4 maskColor = vec4(1.0);
                if (enableMask) {
                    vec2 normalizedCoord = gl_FragCoord.xy / viewportSize;
                    maskColor = texture2D(maskTexture, normalizedCoord);
                }

                vec4 color = texture2D(texture1, texCoord);
                if (enableMask && maskColor.a < 0.5) {
                    discard;
                } else {
                    if(enableColor)
                    {
                        color = vec4(uniformColor.xyz, uniformColor.a * color.a);
                    }
                    color.a = color.a * opa;
                    gl_FragColor = color;
                }
            }
        )";
#endif

namespace
{
unsigned int CompileShader(unsigned int type, const char* src)
{
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        TVPConsoleLog("GLRenderBackend shader compile error: %s", log);
    }
    return shader;
}
} // namespace

GLRenderBackend::~GLRenderBackend()
{
    // 窗口合成资源
    if (program)
    {
        glDeleteProgram(program);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
        program = vao = vbo = ebo = 0;
    }
    // 2D 网格资源
    if (program_)
    {
        glDeleteProgram(program_);
        glDeleteVertexArrays(1, &vao_);
        glDeleteBuffers(1, &vbo_);
        glDeleteBuffers(1, &ibo_);
        program_ = vao_ = vbo_ = ibo_ = 0;
    }
    // 目标（FBO）
    for (Target* t : targets_)
    {
        if (t->fbo)
            glDeleteFramebuffers(1, &t->fbo);
        if (t->colorTex)
            glDeleteTextures(1, &t->colorTex);
        if (t->depthTex)
            glDeleteTextures(1, &t->depthTex);
        delete t;
    }
    targets_.clear();
    // 贴图（窗口贴图与一般贴图共用）
    for (Texture* t : textures_)
    {
        if (t->id)
            glDeleteTextures(1, &t->id);
        delete t;
    }
    textures_.clear();
}

GLRenderBackend::Texture* GLRenderBackend::FindTexture(void* handle) const
{
    for (Texture* t : textures_)
    {
        if (t == handle)
            return t;
    }
    return nullptr;
}

GLRenderBackend::Target* GLRenderBackend::FindTarget(void* handle) const
{
    for (Target* t : targets_)
    {
        if (t == handle)
            return t;
    }
    return nullptr;
}

void GLRenderBackend::FetchInfo()
{
    const GLubyte* vendor = glGetString(GL_VENDOR);
    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    const GLubyte* glslVersion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    ttstr log(TJS_N("OpenGL Vendor: "));
    log += ttstr((const char*)vendor) + TJS_N(" / ") + ttstr((const char*)renderer);
    TVPAddImportantLog(log);
    log = TJS_N("OpenGL Version: ") + ttstr((const char*)version);
    TVPAddImportantLog(log);
    log = TJS_N("GLSL Version: ") + ttstr((const char*)glslVersion);
    TVPAddImportantLog(log);

    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);
    log = TJS_N("OpenGL extensions (") + ttstr(numExtensions) + TJS_N("):");
    for (int i = 0; i < numExtensions; i++)
    {
        const GLubyte* ext = glGetStringi(GL_EXTENSIONS, i);
        sTVPGLExtensions.emplace(std::string((const char*)ext));
        log += " " + ttstr((const char*)ext);
    }
    TVPAddImportantLog(log);
}

//---------------------------------------------------------------------------
// 窗口合成（iTVPRenderBackend）
//---------------------------------------------------------------------------
bool GLRenderBackend::EnsureResources()
{
    if (program != 0 && glIsProgram(program) == GL_TRUE)
        return true;

    // 创建 shader program
    GLuint vs = CompileShader(GL_VERTEX_SHADER, kWindowVertexShaderSrc);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, kWindowFragmentShaderSrc);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        TVPConsoleLog("Program link error: %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // 顶点数据
    float vertices[] = {
        // 位置          // 纹理坐标
        0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // 左下
        1.0f, 0.0f, 0.0f, 1.0f, 1.0f, // 右下
        1.0f, 1.0f, 0.0f, 1.0f, 0.0f, // 右上
        0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // 左上
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    unsigned int indices[] = {
        0, 1, 2, // 第一个三角形
        2, 3, 0  // 第二个三角形
    };
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float),
                          (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // uniform location 只查询一次（原实现每帧每节点查询）
    locTexture = glGetUniformLocation(prog, "texture1");
    locWindowSize = glGetUniformLocation(prog, "windowSize");
    locPosition = glGetUniformLocation(prog, "texture_Position");
    locSize = glGetUniformLocation(prog, "texture_Size");

    program = prog;
    return true;
}

void GLRenderBackend::BeginFrame(int w, int h)
{
    viewportW = w;
    viewportH = h;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClear(GL_COLOR_BUFFER_BIT);
    if (!EnsureResources())
        return;
    glUseProgram(program);
    glViewport(0, 0, w, h);
    glBindVertexArray(vao);
}

void GLRenderBackend::EndFrame()
{
    // 呈现交给平台钩子（SDL_GL_SwapWindow / eglSwapBuffers）
    TVPSwapBuffersBackend();
}

void GLRenderBackend::DrawWindowTexture(void* handle, float posX, float posY, float width, float height)
{
    Texture* tex = FindTexture(handle);
    if (!tex)
        return;
    int w = viewportW, h = viewportH;
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex->id);
    glUniform1i(locTexture, 0);
    glUniform2f(locWindowSize, w, h);
    // 与原始实现一致：以归一化坐标传给 shader
    glUniform2f(locPosition, posX / w, posY / h);
    glUniform2f(locSize, width / w, height / h);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

//---------------------------------------------------------------------------
// 贴图（窗口贴图与一般贴图共用同一实现）
//---------------------------------------------------------------------------
void* GLRenderBackend::CreateWindowTexture(int width, int height)
{
    return CreateTexture(width, height);
}

void GLRenderBackend::UpdateWindowTexture(void* handle, const uint8_t* buff, int width, int height, int pitch)
{
    UpdateTexture(handle, buff, width, height, pitch);
}

void GLRenderBackend::DestroyWindowTexture(void* handle)
{
    DestroyTexture(handle);
}

//---------------------------------------------------------------------------
// 2D 网格渲染（一般贴图 + 离屏网格绘制）
//---------------------------------------------------------------------------
bool GLRenderBackend::EnsureMeshProgram()
{
    if (program_ != 0 && glIsProgram(program_) == GL_TRUE)
        return true;

    unsigned int vs = CompileShader(GL_VERTEX_SHADER, kMeshVertexShaderSrc);
    unsigned int fs = CompileShader(GL_FRAGMENT_SHADER, kMeshFragmentShaderSrc);
    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    int success = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    if (!success)
    {
        char log[512];
        glGetProgramInfoLog(prog, 512, nullptr, log);
        TVPConsoleLog("GLRenderBackend mesh program link error: %s", log);
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    // uniform location 只查询一次
    locTexture_ = glGetUniformLocation(prog, "texture1");
    locMask_ = glGetUniformLocation(prog, "maskTexture");
    locEnableMask_ = glGetUniformLocation(prog, "enableMask");
    locEnableColor_ = glGetUniformLocation(prog, "enableColor");
    locOpa_ = glGetUniformLocation(prog, "opa");
    locUniformColor_ = glGetUniformLocation(prog, "uniformColor");
    locViewportSize_ = glGetUniformLocation(prog, "viewportSize");

    program_ = prog;
    return true;
}

//---------------------------------------------------------------------------
// 目标（FBO）
//---------------------------------------------------------------------------
void* GLRenderBackend::CreateTarget(int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    Target* target = new Target();
    target->width = width;
    target->height = height;
    target->readback.resize((size_t)width * height * 4);

    // 颜色纹理
    glGenTextures(1, &target->colorTex);
    glBindTexture(GL_TEXTURE_2D, target->colorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 深度纹理
    glGenTextures(1, &target->depthTex);
    glBindTexture(GL_TEXTURE_2D, target->depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_INT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // FBO
    glGenFramebuffers(1, &target->fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target->colorTex, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, target->depthTex, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
        TVPConsoleLog("GLRenderBackend framebuffer incomplete!");
    }

    targets_.push_back(target);
    return target;
}

void GLRenderBackend::DestroyTarget(void* handle)
{
    Target* target = FindTarget(handle);
    if (!target)
        return;
    if (currentTarget_ == target)
        currentTarget_ = nullptr;
    if (maskTarget_ == target)
        maskTarget_ = nullptr;
    for (size_t i = 0; i < targets_.size(); i++)
    {
        if (targets_[i] == target)
        {
            targets_.erase(targets_.begin() + i);
            break;
        }
    }
    glDeleteFramebuffers(1, &target->fbo);
    glDeleteTextures(1, &target->colorTex);
    glDeleteTextures(1, &target->depthTex);
    delete target;
}

void GLRenderBackend::SetTarget(void* handle)
{
    if (!EnsureMeshProgram())
        return;
    currentTarget_ = FindTarget(handle);
    if (!currentTarget_)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, currentTarget_->fbo);
    glViewport(0, 0, currentTarget_->width, currentTarget_->height);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
#if defined(_KRKRSDL3_GL) && _KRKRSDL3_GL
    glClearDepth(-1.0f);
#else
    glClearDepthf(-1.0f);
#endif
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_GEQUAL);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    glUseProgram(program_);
    glBindVertexArray(vao_);
}

void GLRenderBackend::ClearTarget(bool clearColor)
{
    if (!currentTarget_)
        return;
    glBindFramebuffer(GL_FRAMEBUFFER, currentTarget_->fbo);
    if (clearColor)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    else
        glClear(GL_DEPTH_BUFFER_BIT);
}

uint8_t* GLRenderBackend::LockTarget(void* handle, int& pitch)
{
    Target* target = FindTarget(handle);
    if (!target)
        return nullptr;
    glBindFramebuffer(GL_FRAMEBUFFER, target->fbo);
    glReadPixels(0, 0, target->width, target->height, GL_RGBA, GL_UNSIGNED_BYTE,
                 target->readback.data());
    pitch = target->width * 4;
    return target->readback.data();
}

void GLRenderBackend::UnlockTarget(void* handle)
{
    (void)handle;
}

//---------------------------------------------------------------------------
// 一般贴图（与窗口贴图共用同一实现）
//---------------------------------------------------------------------------
void* GLRenderBackend::CreateTexture(int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;
    Texture* texture = new Texture();
    texture->width = width;
    texture->height = height;
    glGenTextures(1, &texture->id);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    textures_.push_back(texture);
    return texture;
}

void GLRenderBackend::UpdateTexture(void* handle, const uint8_t* pixels, int width, int height, int pitch)
{
    Texture* texture = FindTexture(handle);
    if (!texture || !pixels)
        return;
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenerateMipmap(GL_TEXTURE_2D); // 与迁移前 emoteplayer 行为一致
}

void GLRenderBackend::DestroyTexture(void* handle)
{
    Texture* texture = FindTexture(handle);
    if (!texture)
        return;
    for (size_t i = 0; i < textures_.size(); i++)
    {
        if (textures_[i] == texture)
        {
            textures_.erase(textures_.begin() + i);
            break;
        }
    }
    glDeleteTextures(1, &texture->id);
    delete texture;
}

//---------------------------------------------------------------------------
// 绘制状态
//---------------------------------------------------------------------------
void GLRenderBackend::SetMask(void* handle)
{
    maskTarget_ = FindTarget(handle);
}

void GLRenderBackend::SetBlendMode(int mode, const float* uniformColor)
{
    blendMode_ = mode;
    skipDraw_ = (mode == 6);
    enableColor_ = false;
    if (mode == 21 && uniformColor)
    {
        enableColor_ = true;
        std::memcpy(uniformColor_, uniformColor, sizeof(uniformColor_));
    }
    if (skipDraw_)
        return;
    switch (mode)
    {
        case 0:
        case 3:
        default:
            glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
            glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
            break;
        case 1:
        case 4:
            glBlendFuncSeparate(GL_DST_COLOR, GL_ONE, GL_ZERO, GL_ONE);
            glBlendEquation(GL_FUNC_ADD);
            break;
        case 21:
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glBlendEquation(GL_FUNC_ADD);
            break;
    }
}

void GLRenderBackend::DrawMesh(const float* vertices,
                               int vertexCount,
                               const uint16_t* indices,
                               int indexCount,
                               void* handle,
                               float opacity)
{
    if (skipDraw_ || !currentTarget_ || !vertices || !indices || vertexCount <= 0 || indexCount <= 0)
        return;
    Texture* texture = FindTexture(handle);
    if (!texture)
        return;
    if (!EnsureMeshProgram())
        return;

    size_t vertexBytes = (size_t)vertexCount * 4 * sizeof(float);
    size_t indexBytes = (size_t)indexCount * sizeof(uint16_t);

    // VBO/IBO（按需增长复用）
    if (vbo_ == 0)
        glGenBuffers(1, &vbo_);
    if (ibo_ == 0)
        glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    if (vertexBytes > vboSize_)
    {
        glBufferData(GL_ARRAY_BUFFER, vertexBytes, vertices, GL_DYNAMIC_DRAW);
        vboSize_ = vertexBytes;
    }
    else
    {
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBytes, vertices);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    if (indexBytes > iboSize_)
    {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBytes, indices, GL_STATIC_DRAW);
        iboSize_ = indexBytes;
    }
    else if (indexCount > 0)
    {
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexBytes, indices);
    }

    // 顶点属性：aPos(location 0, vec2) + aTexCoord(location 1, vec2)，交错 4 float
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glUseProgram(program_);
    glUniform1f(locOpa_, opacity);

    bool useMask = (maskTarget_ != nullptr);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture->id);
    glUniform1i(locTexture_, 0);
    glUniform1i(locEnableMask_, useMask ? GL_TRUE : GL_FALSE);
    glUniform1i(locEnableColor_, enableColor_ ? GL_TRUE : GL_FALSE);
    glUniform4f(locUniformColor_, uniformColor_[0], uniformColor_[1], uniformColor_[2],
                uniformColor_[3]);
    glUniform2f(locViewportSize_, (float)currentTarget_->width, (float)currentTarget_->height);
    if (useMask)
    {
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, maskTarget_->colorTex);
        glUniform1i(locMask_, 1);
    }
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
}

// 兼容保留：历史接口（当前无外部调用者），供扩展检查使用
bool checkGLExtension(const std::string& extname)
{
    return sTVPGLExtensions.find(extname) != sTVPGLExtensions.end();
}
} // namespace krkrsdl3

//---------------------------------------------------------------------------
// 静态注册：合并后端（窗口合成 + 2D 网格）一次性注册
//---------------------------------------------------------------------------
namespace
{
struct GLRenderBackendAutoRegister
{
    GLRenderBackendAutoRegister()
    {
        krkrsdl3::TVPRenderBackendDesc desc;
        desc.name = "opengl";
        desc.description = "OpenGL 3.3 Core / OpenGL ES 3.0";
        desc.probe = nullptr; // 编译进来即可用（上下文由平台入口创建）
        desc.create = nullptr;
        krkrsdl3::TVPRegisterRenderBackend(desc);
    }
} gGLRenderBackendAutoRegister;
} // namespace
