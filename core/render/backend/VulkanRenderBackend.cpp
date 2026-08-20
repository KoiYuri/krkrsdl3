#if defined(_KRKRSDL3_USE_SDL3) && defined(_KRKRSDL3_USE_VULKAN)

#include "VulkanRenderBackend.h"

#include "tjsCommHead.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "Platform.h"
#include "TVPDebug.h"
#include "TVPSettings.h"

#include "shader/vulkan_shaders.h"
#include "shader/vulkan_shaders2d.h"

//---------------------------------------------------------------------------
// Vulkan 渲染后端（合并实现）
//
// 一个类同时承担两个角色（同一接口内，见 backend/RenderBackend.h）：
//   1. 窗口贴图合成后端——上屏呈现。
//   2. 2D 网格渲染器——离屏目标 + 网格绘制（emoteplayer 等插件）。
//
// GPU 后端下窗口贴图与一般贴图是同一套纹理实现
// （CreateWindowTexture == CreateTexture 等），共享设备/描述符池/采样器；
// 窗口合成与 2D 网格各自使用独立的管线/命令缓冲，互不干扰。
//---------------------------------------------------------------------------
namespace krkrsdl3
{
namespace
{
constexpr uint32_t kMaxDescriptorSets = 1024;
constexpr uint32_t kMaxDescriptorCount = 2048;
constexpr uint32_t kVertexCount = 4;
constexpr uint32_t kIndexCount = 6;

constexpr int kPipelineNormal = 0;   // bm 0 / 3 / 默认
constexpr int kPipelineMultiply = 1; // bm 1 / 4
constexpr int kPipelineColor = 2;    // bm 21

// 窗口合成 push constant：position(2) + size(2) + viewport(2)
struct WindowPushConstants
{
    float posX, posY;
    float sizeX, sizeY;
    float viewW, viewH;
};
static_assert(sizeof(WindowPushConstants) == 24, "window push constant layout");

// 2D 网格 push constant：与 vk2d_quad.frag 的布局一致（vec4 + vec2 + 3 floats = 36 字节）
struct MeshPushConstants
{
    float uniformColor[4];
    float viewportX, viewportY;
    float enableMask;  // 0.0 / 1.0
    float enableColor; // 0.0 / 1.0
    float opa;
};
static_assert(sizeof(MeshPushConstants) == 36, "mesh push constant layout");

bool CheckVkResult(VkResult result, const char* what)
{
    if (result != VK_SUCCESS)
    {
        TVPConsoleLog("Vulkan %s failed: %d", what, (int)result);
        return false;
    }
    return true;
}

uint32_t FindHostVisibleMemory(VkPhysicalDevice phys,
                               const VkMemoryRequirements& req,
                               bool coherentOnly)
{
    VkPhysicalDeviceMemoryProperties props;
    vkGetPhysicalDeviceMemoryProperties(phys, &props);
    for (uint32_t i = 0; i < props.memoryTypeCount; i++)
    {
        if (!(req.memoryTypeBits & (1u << i)))
            continue;
        VkMemoryPropertyFlags flags = props.memoryTypes[i].propertyFlags;
        if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        {
            if (!coherentOnly || (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
                return i;
        }
    }
    return 0xFFFFFFFF;
}
} // namespace

class VulkanRenderBackend : public iTVPRenderBackend
{
public:
    VulkanRenderBackend(VkInstance _instance, VkSurfaceKHR _surface)
      : instance_(_instance),
        surface_(_surface)
    {
    }
    ~VulkanRenderBackend() override { Shutdown(); }

    const char* GetName() const override { return "vulkan"; }
    bool IsHardware() const override { return true; }

    // ---- 窗口贴图合成 ----
    bool Initialize();
    void Shutdown();

    void BeginFrame(int winWidth, int winHeight) override;
    void EndFrame() override;

    void* CreateWindowTexture(int width, int height) override;
    void UpdateWindowTexture(void* handle, const uint8_t* buff, int width, int height, int pitch) override;
    void DestroyWindowTexture(void* handle) override;
    void DrawWindowTexture(void* handle, float posX, float posY, float width, float height) override;

    // ---- 2D 网格渲染（一般贴图 + 离屏网格绘制）----
    void* CreateTarget(int width, int height) override;
    void DestroyTarget(void* target) override;
    void SetTarget(void* target) override;
    void ClearTarget(bool clearColor) override;
    uint8_t* LockTarget(void* target, int& pitch) override;
    void UnlockTarget(void* target) override;
    void* CreateTexture(int width, int height) override;
    void UpdateTexture(void* texture, const uint8_t* pixels, int width, int height, int pitch) override;
    void DestroyTexture(void* texture) override;
    void SetMask(void* maskTarget) override;
    void SetBlendMode(int mode, const float* uniformColor) override;
    void DrawMesh(const float* vertices,
                  int vertexCount,
                  const uint16_t* indices,
                  int indexCount,
                  void* texture,
                  float opacity) override;

private:
    // ---- 统一贴图（窗口贴图与一般贴图共用同一实现）----
    struct Texture
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkDescriptorSet set = VK_NULL_HANDLE; // set0（纹理）
        uint8_t* mapped = nullptr;
        int width = 0, height = 0;
        int pitch = 0;
    };
    // 2D 离屏目标
    struct Target
    {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkDescriptorSet maskSet = VK_NULL_HANDLE; // 作为蒙版采样时绑定（set1）
        int width = 0, height = 0;
    };
    Texture* FindTexture(void* handle) const;
    Texture* CreateTextureInternal(int width, int height);
    void UpdateTextureInternal(Texture* texture, const uint8_t* pixels, int width, int height, int pitch);
    void DestroyTextureInternal(Texture* texture);

    // 窗口合成
    bool PickDevice();
    bool CreateDeviceAndQueues();
    bool CreateSwapchain(int width, int height); // 仅交换链 + 图像视图
    void DestroySwapchain();
    bool CreateWindowRenderPass();
    bool CreateWindowFramebuffers(); // 依赖 windowRenderPass_ 与 swapchainViews_
    bool CreateWindowPipeline();
    bool CreateSyncObjects();
    bool CreateWindowVertexBuffer();
    bool CreateSampler();
    void RecreateSwapchainIfNeeded();

    // 2D 网格（惰性初始化）
    bool EnsureMeshResources();
    bool EnsureMeshRenderPasses();
    bool EnsureMeshPipelines();
    bool BeginPass(Target* target, bool clear);
    void EndPass();
    bool EnsureStaging(size_t bytes);
    bool EnsureVertexBuffers(size_t vertexBytes, size_t indexBytes);

    Target* FindTarget(void* handle) const;

    bool initialized_ = false;
    bool swapchainDirty_ = false;

    // 共享设备
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    uint32_t graphicsFamily_ = 0;
    uint32_t presentFamily_ = 0;

    // 交换链（窗口）
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkFormat swapchainFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D swapchainExtent_{};
    std::vector<VkImage> swapchainImages_;
    std::vector<VkImageView> swapchainViews_;
    std::vector<VkFramebuffer> swapchainFramebuffers_;
    uint32_t currentImage_ = 0;

    // 共享描述符/采样器/命令池（窗口与 2D 网格共用）
    VkDescriptorSetLayout textureSetLayout_ = VK_NULL_HANDLE; // set0（纹理）
    VkDescriptorSetLayout maskSetLayout_ = VK_NULL_HANDLE;    // set1（蒙版，2D）
    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;

    // 窗口管线
    VkRenderPass windowRenderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout windowPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline windowPipeline_ = VK_NULL_HANDLE;
    VkBuffer windowVertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory windowVertexMemory_ = VK_NULL_HANDLE;
    VkBuffer windowIndexBuffer_ = VK_NULL_HANDLE;   // 两个三角形 (0,1,2)/(2,3,0)，与 GL 后端一致
    VkDeviceMemory windowIndexMemory_ = VK_NULL_HANDLE;
    VkCommandBuffer frameCommandBuffer_ = VK_NULL_HANDLE;
    VkFence frameFence_ = VK_NULL_HANDLE;
    VkSemaphore imageReady_ = VK_NULL_HANDLE;
    VkSemaphore renderFinished_ = VK_NULL_HANDLE;
    int lastWidth_ = 0, lastHeight_ = 0;
    bool vsync_ = true;

    // 2D 网格管线（惰性创建）
    VkRenderPass meshClearPass_ = VK_NULL_HANDLE; // loadOp CLEAR
    VkRenderPass meshLoadPass_ = VK_NULL_HANDLE;  // loadOp LOAD
    VkPipelineLayout meshPipelineLayout_ = VK_NULL_HANDLE;
    VkPipeline meshPipelines_[3] = {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};
    VkCommandBuffer meshCommandBuffer_ = VK_NULL_HANDLE;
    VkFence meshFence_ = VK_NULL_HANDLE;
    bool meshReady_ = false;
    bool commandActive_ = false;

    VkBuffer meshVertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory meshVertexMemory_ = VK_NULL_HANDLE;
    size_t meshVertexCapacity_ = 0;
    uint8_t* meshVertexMapped_ = nullptr;
    VkBuffer meshIndexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory meshIndexMemory_ = VK_NULL_HANDLE;
    size_t meshIndexCapacity_ = 0;
    uint8_t* meshIndexMapped_ = nullptr;

    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory_ = VK_NULL_HANDLE;
    size_t stagingSize_ = 0;
    uint8_t* stagingMapped_ = nullptr;

    // 空白蒙版（1x1 全白，无蒙版时绑定）
    Texture* blankMask_ = nullptr;

    // 2D 状态
    Target* currentTarget_ = nullptr;
    Target* maskTarget_ = nullptr;
    bool passActive_ = false;
    bool passClear_ = false;
    int blendMode_ = 0;
    bool skipDraw_ = false;
    bool enableColor_ = false;
    float uniformColor_[4] = {0, 0, 0, 0};

    std::vector<Target*> targets_;
    std::vector<Texture*> textures_;
};

//---------------------------------------------------------------------------
// 初始化
//---------------------------------------------------------------------------
bool VulkanRenderBackend::Initialize()
{
    if (initialized_)
        return true;

    vsync_ = TVPSettings.vsync != 0;

    if (!PickDevice())
        return false;
    if (!CreateDeviceAndQueues())
        return false;

    // Vulkan 严格初始化顺序（参考 out/Vulkan-SDL3 实例）：
    //   Instance → Surface → Device → Swapchain → RenderPass → Framebuffers → Pipeline
    // 交换链只依赖 surface；帧缓冲依赖交换链图像视图与 render pass，故在其后创建。
    int w = 0, h = 0;
    TVPGetWindowSizeInPixels(&w, &h);
    if (!CreateSwapchain(w, h))
        return false;
    if (!CreateWindowRenderPass())
        return false;
    if (!CreateWindowFramebuffers())
        return false;
    if (!CreateWindowPipeline())
        return false;
    if (!CreateSyncObjects())
        return false;
    if (!CreateWindowVertexBuffer())
        return false;
    if (!CreateSampler())
        return false;

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = graphicsFamily_;
    if (!CheckVkResult(vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_), "CreateCommandPool"))
        return false;

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (!CheckVkResult(vkAllocateCommandBuffers(device_, &allocInfo, &frameCommandBuffer_), "AllocateCommandBuffers"))
        return false;

    TVPConsoleLog("Vulkan backend initialized: %s / %s", "Vulkan 1.0", "quad compositor + 2D mesh");
    initialized_ = true;
    return true;
}

bool VulkanRenderBackend::PickDevice()
{
    uint32_t deviceCount = 0;
    if (!CheckVkResult(vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr), "EnumeratePhysicalDevices") ||
        deviceCount == 0)
    {
        TVPConsoleLog("No Vulkan physical devices found");
        return false;
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    // 优先选择独立显卡，其次集成显卡（简单起见取第一个满足条件的）
    for (VkPhysicalDevice dev : devices)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(dev, &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU || devices.size() == 1)
        {
            physicalDevice_ = dev;
            break;
        }
    }
    if (physicalDevice_ == VK_NULL_HANDLE)
        physicalDevice_ = devices[0];

    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physicalDevice_, &props);
    TVPConsoleLog("Vulkan device: %s", props.deviceName);
    return true;
}

bool VulkanRenderBackend::CreateDeviceAndQueues()
{
    uint32_t familyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, nullptr);
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice_, &familyCount, families.data());

    // 找图形队列族与呈现队列族
    int graphicsFamily = -1, presentFamily = -1;
    for (uint32_t i = 0; i < familyCount; i++)
    {
        if (families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsFamily = (int)i;
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_, &presentSupport);
            if (presentSupport)
            {
                presentFamily = (int)i;
                break; // 同一队列族同时支持图形与呈现
            }
        }
    }
    if (graphicsFamily < 0)
    {
        // 图形与呈现族不同
        for (uint32_t i = 0; i < familyCount && presentFamily < 0; i++)
        {
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice_, i, surface_, &presentSupport);
            if (presentSupport)
                presentFamily = (int)i;
        }
        if (graphicsFamily < 0 || presentFamily < 0)
        {
            TVPConsoleLog("No suitable Vulkan queue families");
            return false;
        }
    }
    else if (presentFamily < 0)
    {
        presentFamily = graphicsFamily; // 同族
    }

    graphicsFamily_ = (uint32_t)graphicsFamily;
    presentFamily_ = (uint32_t)presentFamily;

    const float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueInfos;
    auto addQueueInfo = [&](uint32_t family) {
        for (auto& qi : queueInfos)
            if (qi.queueFamilyIndex == family)
                return;
        VkDeviceQueueCreateInfo qi{};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.queueFamilyIndex = family;
        qi.queueCount = 1;
        qi.pQueuePriorities = &queuePriority;
        queueInfos.push_back(qi);
    };
    addQueueInfo(graphicsFamily_);
    addQueueInfo(presentFamily_);

    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkDeviceCreateInfo deviceInfo{};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = (uint32_t)queueInfos.size();
    deviceInfo.pQueueCreateInfos = queueInfos.data();
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;

    if (!CheckVkResult(vkCreateDevice(physicalDevice_, &deviceInfo, nullptr, &device_), "CreateDevice"))
        return false;

    vkGetDeviceQueue(device_, graphicsFamily_, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, presentFamily_, 0, &presentQueue_);
    return true;
}

//---------------------------------------------------------------------------
// 交换链
//---------------------------------------------------------------------------
bool VulkanRenderBackend::CreateSwapchain(int width, int height)
{
    if (width <= 0 || height <= 0)
        return false;

    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice_, surface_, &caps);

    // 格式
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice_, surface_, &formatCount, formats.data());
    swapchainFormat_ = VK_FORMAT_B8G8R8A8_UNORM;
    for (const auto& f : formats)
    {
        if (f.format == VK_FORMAT_B8G8R8A8_UNORM || f.format == VK_FORMAT_R8G8B8A8_UNORM)
        {
            swapchainFormat_ = f.format;
            break;
        }
    }

    // 呈现模式
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice_, surface_, &presentModeCount, presentModes.data());
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR; // 有 vsync，保证可用
    if (!vsync_)
    {
        if (std::find(presentModes.begin(), presentModes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != presentModes.end())
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        else
            presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    }

    // 尺寸（限制在能力范围内）
    VkExtent2D extent{};
    if (caps.currentExtent.width != 0xFFFFFFFF)
        extent = caps.currentExtent;
    else
    {
        extent.width = (uint32_t)std::max(1, std::min(width, (int)caps.maxImageExtent.width));
        extent.height = (uint32_t)std::max(1, std::min(height, (int)caps.maxImageExtent.height));
    }
    swapchainExtent_ = extent;

    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR swapInfo{};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = surface_;
    swapInfo.minImageCount = imageCount;
    swapInfo.imageFormat = swapchainFormat_;
    swapInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapInfo.imageExtent = swapchainExtent_;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (graphicsFamily_ != presentFamily_)
    {
        uint32_t queueFamilies[] = {graphicsFamily_, presentFamily_};
        swapInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapInfo.queueFamilyIndexCount = 2;
        swapInfo.pQueueFamilyIndices = queueFamilies;
    }
    else
    {
        swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    swapInfo.preTransform = caps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = presentMode;
    swapInfo.clipped = VK_TRUE;

    if (!CheckVkResult(vkCreateSwapchainKHR(device_, &swapInfo, nullptr, &swapchain_), "CreateSwapchain"))
        return false;

    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, nullptr);
    swapchainImages_.resize(imageCount);
    vkGetSwapchainImagesKHR(device_, swapchain_, &imageCount, swapchainImages_.data());

    // 图像视图（帧缓冲在 CreateWindowFramebuffers 中创建——
    // 需先有 render pass，见初始化顺序注释）
    swapchainViews_.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++)
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = swapchainImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = swapchainFormat_;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.layerCount = 1;
        if (!CheckVkResult(vkCreateImageView(device_, &viewInfo, nullptr, &swapchainViews_[i]), "CreateImageView"))
            return false;
    }
    return true;
}

// 帧缓冲：必须在 windowRenderPass_ 与交换链图像视图创建之后调用
bool VulkanRenderBackend::CreateWindowFramebuffers()
{
    if (windowRenderPass_ == VK_NULL_HANDLE || swapchainViews_.empty())
        return false;
    swapchainFramebuffers_.resize(swapchainViews_.size());
    for (size_t i = 0; i < swapchainViews_.size(); i++)
    {
        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = windowRenderPass_;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = &swapchainViews_[i];
        fbInfo.width = swapchainExtent_.width;
        fbInfo.height = swapchainExtent_.height;
        fbInfo.layers = 1;
        if (!CheckVkResult(vkCreateFramebuffer(device_, &fbInfo, nullptr, &swapchainFramebuffers_[i]), "CreateFramebuffer"))
            return false;
    }
    return true;
}

void VulkanRenderBackend::DestroySwapchain()
{
    if (device_ == VK_NULL_HANDLE)
        return;
    vkDeviceWaitIdle(device_);
    for (auto fb : swapchainFramebuffers_)
        if (fb)
            vkDestroyFramebuffer(device_, fb, nullptr);
    for (auto v : swapchainViews_)
        if (v)
            vkDestroyImageView(device_, v, nullptr);
    swapchainFramebuffers_.clear();
    swapchainViews_.clear();
    swapchainImages_.clear();
    if (swapchain_)
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    swapchain_ = VK_NULL_HANDLE;
}

void VulkanRenderBackend::RecreateSwapchainIfNeeded()
{
    int w = 0, h = 0;
    TVPGetWindowSizeInPixels(&w, &h);
    if (w <= 0 || h <= 0)
        return;
    if (swapchain_ && (uint32_t)w == swapchainExtent_.width && (uint32_t)h == swapchainExtent_.height && !swapchainDirty_)
        return;
    swapchainDirty_ = false;
    DestroySwapchain();
    if (!CreateSwapchain(w, h) || !CreateWindowFramebuffers())
    {
        TVPConsoleLog("Vulkan swapchain recreation failed");
        swapchainDirty_ = true;
    }
}

//---------------------------------------------------------------------------
// 渲染管线（窗口合成）
//---------------------------------------------------------------------------
bool VulkanRenderBackend::CreateWindowRenderPass()
{
    VkAttachmentDescription attachment{};
    attachment.format = swapchainFormat_;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &attachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;
    return CheckVkResult(vkCreateRenderPass(device_, &rpInfo, nullptr, &windowRenderPass_), "CreateRenderPass");
}

bool VulkanRenderBackend::CreateWindowPipeline()
{
    // 描述符集布局：set0/binding0 = combined image sampler（纹理，窗口与 2D 共用）
    VkDescriptorSetLayoutBinding binding{};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;
    if (!CheckVkResult(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &textureSetLayout_),
                       "CreateDescriptorSetLayout"))
        return false;

    // push constant（顶点阶段）
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(WindowPushConstants);

    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &textureSetLayout_;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pushRange;
    if (!CheckVkResult(vkCreatePipelineLayout(device_, &plInfo, nullptr, &windowPipelineLayout_), "CreatePipelineLayout"))
        return false;

    VkShaderModule vertModule = VK_NULL_HANDLE, fragModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo vsInfo{};
    vsInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vsInfo.codeSize = sizeof(kVulkanVertSpv);
    vsInfo.pCode = kVulkanVertSpv;
    VkShaderModuleCreateInfo fsInfo{};
    fsInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fsInfo.codeSize = sizeof(kVulkanFragSpv);
    fsInfo.pCode = kVulkanFragSpv;
    if (!CheckVkResult(vkCreateShaderModule(device_, &vsInfo, nullptr, &vertModule), "CreateVertexShader") ||
        !CheckVkResult(vkCreateShaderModule(device_, &fsInfo, nullptr, &fragModule), "CreateFragmentShader"))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    // 顶点输入：pos(2f) + uv(2f)
    VkVertexInputBindingDescription vertexBinding{};
    vertexBinding.binding = 0;
    vertexBinding.stride = 4 * sizeof(float);
    vertexBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription vertexAttribs[2]{};
    vertexAttribs[0].location = 0;
    vertexAttribs[0].binding = 0;
    vertexAttribs[0].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttribs[0].offset = 0;
    vertexAttribs[1].location = 1;
    vertexAttribs[1].binding = 0;
    vertexAttribs[1].format = VK_FORMAT_R32G32_SFLOAT;
    vertexAttribs[1].offset = 2 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &vertexBinding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = vertexAttribs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // 无混合（与 GL 后端行为一致）
    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                     VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    blendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlend{};
    colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlend.attachmentCount = 1;
    colorBlend.pAttachments = &blendAttachment;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = stages;
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisample;
    pipelineInfo.pColorBlendState = &colorBlend;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = windowPipelineLayout_;
    pipelineInfo.renderPass = windowRenderPass_;
    pipelineInfo.subpass = 0;

    VkResult result = vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &windowPipeline_);
    vkDestroyShaderModule(device_, vertModule, nullptr);
    vkDestroyShaderModule(device_, fragModule, nullptr);
    if (!CheckVkResult(result, "CreateGraphicsPipelines"))
        return false;

    // 描述符池（窗口与 2D 网格共用）
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = kMaxDescriptorCount;
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = kMaxDescriptorSets;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    return CheckVkResult(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_), "CreateDescriptorPool");
}

bool VulkanRenderBackend::CreateSyncObjects()
{
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VkSemaphoreCreateInfo semInfo{};
    semInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    return CheckVkResult(vkCreateFence(device_, &fenceInfo, nullptr, &frameFence_), "CreateFence") &&
           CheckVkResult(vkCreateSemaphore(device_, &semInfo, nullptr, &imageReady_), "CreateSemaphore") &&
           CheckVkResult(vkCreateSemaphore(device_, &semInfo, nullptr, &renderFinished_), "CreateSemaphore");
}

bool VulkanRenderBackend::CreateWindowVertexBuffer()
{
    // 单位四边形（与 GL 后端同一套顶点/纹理坐标）
    float vertices[] = {
        // 位置   // 纹理坐标
        0.f, 0.f, 0.f, 1.f, // 左下
        1.f, 0.f, 1.f, 1.f, // 右下
        1.f, 1.f, 1.f, 0.f, // 右上
        0.f, 1.f, 0.f, 0.f, // 左上
    };
    const VkDeviceSize size = sizeof(vertices);

    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!CheckVkResult(vkCreateBuffer(device_, &bufferInfo, nullptr, &windowVertexBuffer_), "CreateVertexBuffer"))
        return false;

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device_, windowVertexBuffer_, &memReq);
    uint32_t memoryType = FindHostVisibleMemory(physicalDevice_, memReq, false);
    if (memoryType == 0xFFFFFFFF)
        return false;

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryType;
    if (!CheckVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &windowVertexMemory_), "AllocateVertexMemory"))
        return false;
    vkBindBufferMemory(device_, windowVertexBuffer_, windowVertexMemory_, 0);

    void* data = nullptr;
    vkMapMemory(device_, windowVertexMemory_, 0, size, 0, &data);
    memcpy(data, vertices, size);
    vkUnmapMemory(device_, windowVertexMemory_);

    // 索引：两个三角形（与 GL 后端的 EBO 一致：0,1,2 / 2,3,0）。
    // 注意：不能无索引 vkCmdDraw(4)——TRIANGLE_LIST 下 4 顶点只会覆盖右半屏。
    uint16_t indices[] = {0, 1, 2, 2, 3, 0};
    const VkDeviceSize indexSize = sizeof(indices);
    VkBufferCreateInfo indexInfo{};
    indexInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexInfo.size = indexSize;
    indexInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!CheckVkResult(vkCreateBuffer(device_, &indexInfo, nullptr, &windowIndexBuffer_), "CreateWindowIndexBuffer"))
        return false;
    VkMemoryRequirements indexMemReq;
    vkGetBufferMemoryRequirements(device_, windowIndexBuffer_, &indexMemReq);
    uint32_t indexMemoryType = FindHostVisibleMemory(physicalDevice_, indexMemReq, false);
    if (indexMemoryType == 0xFFFFFFFF)
        return false;
    VkMemoryAllocateInfo indexAlloc{};
    indexAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    indexAlloc.allocationSize = indexMemReq.size;
    indexAlloc.memoryTypeIndex = indexMemoryType;
    if (!CheckVkResult(vkAllocateMemory(device_, &indexAlloc, nullptr, &windowIndexMemory_), "AllocateWindowIndexMemory"))
        return false;
    vkBindBufferMemory(device_, windowIndexBuffer_, windowIndexMemory_, 0);
    void* indexData = nullptr;
    vkMapMemory(device_, windowIndexMemory_, 0, indexSize, 0, &indexData);
    memcpy(indexData, indices, indexSize);
    vkUnmapMemory(device_, windowIndexMemory_);
    return true;
}

bool VulkanRenderBackend::CreateSampler()
{
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.maxLod = 0.0f;
    return CheckVkResult(vkCreateSampler(device_, &samplerInfo, nullptr, &sampler_), "CreateSampler");
}

//---------------------------------------------------------------------------
// 帧控制（窗口）
//---------------------------------------------------------------------------
void VulkanRenderBackend::BeginFrame(int winWidth, int winHeight)
{
    if (!initialized_)
        return;
    (void)winWidth;
    (void)winHeight;
    RecreateSwapchainIfNeeded();
    if (!swapchain_ || swapchainDirty_)
        return;

    // 等待上一帧完成
    vkWaitForFences(device_, 1, &frameFence_, VK_TRUE, UINT64_MAX);
    vkResetFences(device_, 1, &frameFence_);

    VkResult result = vkAcquireNextImageKHR(device_, swapchain_, UINT64_MAX, imageReady_, VK_NULL_HANDLE, &currentImage_);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        swapchainDirty_ = true;
        return;
    }
    if (!CheckVkResult(result, "AcquireNextImage"))
        return;

    vkResetCommandBuffer(frameCommandBuffer_, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    if (!CheckVkResult(vkBeginCommandBuffer(frameCommandBuffer_, &beginInfo), "BeginCommandBuffer"))
        return;

    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 0.0f}}};
    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = windowRenderPass_;
    rpBegin.framebuffer = swapchainFramebuffers_[currentImage_];
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = swapchainExtent_;
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearColor;
    vkCmdBeginRenderPass(frameCommandBuffer_, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)swapchainExtent_.width;
    viewport.height = (float)swapchainExtent_.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(frameCommandBuffer_, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent_;
    vkCmdSetScissor(frameCommandBuffer_, 0, 1, &scissor);

    vkCmdBindPipeline(frameCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, windowPipeline_);
    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(frameCommandBuffer_, 0, 1, &windowVertexBuffer_, &offset);
}

void VulkanRenderBackend::DrawWindowTexture(void* handle, float posX, float posY, float width, float height)
{
    if (!initialized_ || !swapchain_ || !handle)
        return;
    Texture* tex = FindTexture(handle);
    if (!tex)
        return;

    WindowPushConstants pc{};
    pc.posX = posX;
    pc.posY = posY;
    pc.sizeX = width;
    pc.sizeY = height;
    pc.viewW = (float)swapchainExtent_.width;
    pc.viewH = (float)swapchainExtent_.height;
    vkCmdPushConstants(frameCommandBuffer_, windowPipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(pc), &pc);

    vkCmdBindDescriptorSets(frameCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, windowPipelineLayout_, 0, 1,
                            &tex->set, 0, nullptr);
    // 索引绘制：两个三角形覆盖整个四边形（与 GL 后端 EBO 一致）
    vkCmdBindIndexBuffer(frameCommandBuffer_, windowIndexBuffer_, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(frameCommandBuffer_, 6, 1, 0, 0, 0);
}

void VulkanRenderBackend::EndFrame()
{
    if (!initialized_ || !swapchain_ || swapchainDirty_)
        return;

    vkCmdEndRenderPass(frameCommandBuffer_);
    if (!CheckVkResult(vkEndCommandBuffer(frameCommandBuffer_), "EndCommandBuffer"))
        return;

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &imageReady_;
    submitInfo.pWaitDstStageMask = &waitStage;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &frameCommandBuffer_;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &renderFinished_;
    if (!CheckVkResult(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, frameFence_), "QueueSubmit"))
        return;

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &renderFinished_;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &currentImage_;
    VkResult result = vkQueuePresentKHR(presentQueue_, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        swapchainDirty_ = true;
    else
        CheckVkResult(result, "QueuePresent");
}

//---------------------------------------------------------------------------
// 统一贴图（窗口贴图与一般贴图共用同一实现）
//---------------------------------------------------------------------------
VulkanRenderBackend::Texture* VulkanRenderBackend::FindTexture(void* handle) const
{
    for (Texture* tex : textures_)
    {
        if (tex == handle)
            return tex;
    }
    return nullptr;
}

VulkanRenderBackend::Texture* VulkanRenderBackend::CreateTextureInternal(int width, int height)
{
    if (width <= 0 || height <= 0)
        return nullptr;

    Texture* texture = new Texture();
    texture->width = width;
    texture->height = height;

    // 线性布局 + 主机可见（与"每帧全量上传"的数据流一致；行距查询驱动）
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_LINEAR;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!CheckVkResult(vkCreateImage(device_, &imageInfo, nullptr, &texture->image), "CreateTextureImage"))
    {
        delete texture;
        return nullptr;
    }
    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device_, texture->image, &memReq);
    uint32_t memoryType = FindHostVisibleMemory(physicalDevice_, memReq, true);
    if (memoryType == 0xFFFFFFFF)
    {
        vkDestroyImage(device_, texture->image, nullptr);
        delete texture;
        return nullptr;
    }
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryType;
    if (!CheckVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &texture->memory), "AllocateTextureMemory"))
    {
        vkDestroyImage(device_, texture->image, nullptr);
        delete texture;
        return nullptr;
    }
    vkBindImageMemory(device_, texture->image, texture->memory, 0);

    // 行距查询
    VkImageSubresource subres{};
    subres.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    subres.mipLevel = 0;
    subres.arrayLayer = 0;
    VkSubresourceLayout subresLayout;
    vkGetImageSubresourceLayout(device_, texture->image, &subres, &subresLayout);
    texture->pitch = (int)subresLayout.rowPitch;
    vkMapMemory(device_, texture->memory, 0, memReq.size, 0, (void**)&texture->mapped);

    // 一次性转换到 GENERAL（采样布局）
    VkCommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = commandPool_;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    VkCommandBuffer cmd;
    if (!CheckVkResult(vkAllocateCommandBuffers(device_, &cmdAlloc, &cmd), "AllocateBarrierCmd"))
    {
        vkFreeMemory(device_, texture->memory, nullptr);
        vkDestroyImage(device_, texture->image, nullptr);
        delete texture;
        return nullptr;
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = texture->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);

    // 视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = texture->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (!CheckVkResult(vkCreateImageView(device_, &viewInfo, nullptr, &texture->view), "CreateTextureView"))
    {
        DestroyTextureInternal(texture);
        return nullptr;
    }

    // set0 描述符
    VkDescriptorSetAllocateInfo descAlloc{};
    descAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAlloc.descriptorPool = descriptorPool_;
    descAlloc.descriptorSetCount = 1;
    descAlloc.pSetLayouts = &textureSetLayout_;
    if (!CheckVkResult(vkAllocateDescriptorSets(device_, &descAlloc, &texture->set), "AllocateTextureSet"))
    {
        // 池耗尽：重建并重试一次
        vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = kMaxDescriptorCount * 2;
        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets = kMaxDescriptorSets * 2;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        if (!CheckVkResult(vkCreateDescriptorPool(device_, &poolInfo, nullptr, &descriptorPool_), "RecreateDescriptorPool") ||
            !CheckVkResult(vkAllocateDescriptorSets(device_, &descAlloc, &texture->set), "AllocateTextureSet"))
        {
            DestroyTextureInternal(texture);
            return nullptr;
        }
    }
    VkDescriptorImageInfo imageInfoDesc{};
    imageInfoDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfoDesc.imageView = texture->view;
    imageInfoDesc.sampler = sampler_;
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = texture->set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfoDesc;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    textures_.push_back(texture);
    return texture;
}

void VulkanRenderBackend::UpdateTextureInternal(Texture* texture, const uint8_t* pixels, int width, int height, int pitch)
{
    if (!texture || !texture->mapped || !pixels)
        return;
    // 线性布局图像按行写入（pitch 可能含对齐填充）
    for (int y = 0; y < height; y++)
    {
        memcpy(texture->mapped + (size_t)y * texture->pitch, pixels + (size_t)y * pitch, (size_t)width * 4);
    }
}

void VulkanRenderBackend::DestroyTextureInternal(Texture* texture)
{
    if (!texture)
        return;
    if (device_ == VK_NULL_HANDLE)
    {
        // 设备已销毁时仅回收对象
        for (size_t i = 0; i < textures_.size(); i++)
        {
            if (textures_[i] == texture)
            {
                textures_.erase(textures_.begin() + i);
                break;
            }
        }
        delete texture;
        return;
    }
    vkDeviceWaitIdle(device_);
    for (size_t i = 0; i < textures_.size(); i++)
    {
        if (textures_[i] == texture)
        {
            textures_.erase(textures_.begin() + i);
            break;
        }
    }
    if (texture->set)
        vkFreeDescriptorSets(device_, descriptorPool_, 1, &texture->set);
    if (texture->view)
        vkDestroyImageView(device_, texture->view, nullptr);
    if (texture->memory)
        vkFreeMemory(device_, texture->memory, nullptr);
    if (texture->image)
        vkDestroyImage(device_, texture->image, nullptr);
    delete texture;
}

//---------------------------------------------------------------------------
// 窗口贴图（iTVPRenderBackend）：与一般贴图同一实现
//---------------------------------------------------------------------------
void* VulkanRenderBackend::CreateWindowTexture(int width, int height)
{
    if (!initialized_)
        return nullptr;
    return CreateTextureInternal(width, height);
}

void VulkanRenderBackend::UpdateWindowTexture(void* handle, const uint8_t* buff, int width, int height, int pitch)
{
    UpdateTextureInternal(FindTexture(handle), buff, width, height, pitch);
}

void VulkanRenderBackend::DestroyWindowTexture(void* handle)
{
    DestroyTextureInternal(FindTexture(handle));
}

//---------------------------------------------------------------------------
// 2D 网格（一般贴图 + 离屏网格绘制）
//---------------------------------------------------------------------------
bool VulkanRenderBackend::EnsureMeshResources()
{
    if (meshReady_)
        return true;
    if (!initialized_ || device_ == VK_NULL_HANDLE)
        return false;

    // 共享设备/命令池/采样器/描述符池/纹理布局由窗口侧创建，这里只建网格专属资源

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    if (!CheckVkResult(vkAllocateCommandBuffers(device_, &allocInfo, &meshCommandBuffer_), "AllocateMeshCommandBuffer"))
        return false;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    if (!CheckVkResult(vkCreateFence(device_, &fenceInfo, nullptr, &meshFence_), "CreateMeshFence"))
        return false;

    // 蒙版布局（set1，与 set0 纹理布局相同）
    VkDescriptorSetLayoutBinding bindings[1]{};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = bindings;
    if (!CheckVkResult(vkCreateDescriptorSetLayout(device_, &layoutInfo, nullptr, &maskSetLayout_), "CreateMaskLayout"))
        return false;

    if (!EnsureMeshRenderPasses() || !EnsureMeshPipelines())
        return false;

    // 空白蒙版（1x1 全白）
    blankMask_ = CreateTextureInternal(1, 1);
    if (!blankMask_)
        return false;
    uint8_t white[4] = {255, 255, 255, 255};
    UpdateTextureInternal(blankMask_, white, 1, 1, 4);

    meshReady_ = true;
    return true;
}

bool VulkanRenderBackend::EnsureMeshRenderPasses()
{
    VkAttachmentDescription attachment{};
    attachment.format = VK_FORMAT_R8G8B8A8_UNORM;
    attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // 两个变体仅 loadOp 不同
    attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    attachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkAttachmentReference colorRef{};
    colorRef.attachment = 0;
    colorRef.layout = VK_IMAGE_LAYOUT_GENERAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorRef;

    VkRenderPassCreateInfo rpInfo{};
    rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpInfo.attachmentCount = 1;
    rpInfo.pAttachments = &attachment;
    rpInfo.subpassCount = 1;
    rpInfo.pSubpasses = &subpass;

    if (!CheckVkResult(vkCreateRenderPass(device_, &rpInfo, nullptr, &meshClearPass_), "CreateClearRenderPass"))
        return false;
    attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    if (!CheckVkResult(vkCreateRenderPass(device_, &rpInfo, nullptr, &meshLoadPass_), "CreateLoadRenderPass"))
        return false;
    return true;
}

bool VulkanRenderBackend::EnsureMeshPipelines()
{
    VkPushConstantRange pushRange{};
    pushRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushRange.offset = 0;
    pushRange.size = sizeof(MeshPushConstants);

    VkDescriptorSetLayout layouts[2] = {textureSetLayout_, maskSetLayout_};
    VkPipelineLayoutCreateInfo plInfo{};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 2;
    plInfo.pSetLayouts = layouts;
    plInfo.pushConstantRangeCount = 1;
    plInfo.pPushConstantRanges = &pushRange;
    if (!CheckVkResult(vkCreatePipelineLayout(device_, &plInfo, nullptr, &meshPipelineLayout_), "CreatePipelineLayout"))
        return false;

    VkShaderModule vertModule = VK_NULL_HANDLE, fragModule = VK_NULL_HANDLE;
    VkShaderModuleCreateInfo vsInfo{};
    vsInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    vsInfo.codeSize = sizeof(kVulkan2DVertSpv);
    vsInfo.pCode = kVulkan2DVertSpv;
    VkShaderModuleCreateInfo fsInfo{};
    fsInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    fsInfo.codeSize = sizeof(kVulkan2DFragSpv);
    fsInfo.pCode = kVulkan2DFragSpv;
    if (!CheckVkResult(vkCreateShaderModule(device_, &vsInfo, nullptr, &vertModule), "CreateVertexShader") ||
        !CheckVkResult(vkCreateShaderModule(device_, &fsInfo, nullptr, &fragModule), "CreateFragmentShader"))
    {
        return false;
    }

    VkPipelineShaderStageCreateInfo stages[2]{};
    stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    stages[0].module = vertModule;
    stages[0].pName = "main";
    stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    stages[1].module = fragModule;
    stages[1].pName = "main";

    VkVertexInputBindingDescription binding{};
    binding.binding = 0;
    binding.stride = 4 * sizeof(float);
    binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    VkVertexInputAttributeDescription attribs[2]{};
    attribs[0].location = 0;
    attribs[0].binding = 0;
    attribs[0].format = VK_FORMAT_R32G32_SFLOAT;
    attribs[0].offset = 0;
    attribs[1].location = 1;
    attribs[1].binding = 0;
    attribs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attribs[1].offset = 2 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &binding;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attribs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample{};
    multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE; // 原 GL 路径深度测试恒通过，等价省略
    depthStencil.depthWriteEnable = VK_FALSE;

    VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    // 混合变体：与 GL 的 bm 映射一致
    VkPipelineColorBlendAttachmentState blendAttachments[3]{};
    // bm 0/3：SRC_ALPHA / ONE_MINUS_SRC_ALPHA（RGB）+ ONE/ONE（A，MAX 方程）
    blendAttachments[kPipelineNormal].blendEnable = VK_TRUE;
    blendAttachments[kPipelineNormal].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachments[kPipelineNormal].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachments[kPipelineNormal].colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachments[kPipelineNormal].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachments[kPipelineNormal].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachments[kPipelineNormal].alphaBlendOp = VK_BLEND_OP_MAX;
    blendAttachments[kPipelineNormal].colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                                                      VK_COLOR_COMPONENT_G_BIT |
                                                      VK_COLOR_COMPONENT_B_BIT |
                                                      VK_COLOR_COMPONENT_A_BIT;
    // bm 1/4：DST_COLOR / ONE
    blendAttachments[kPipelineMultiply] = blendAttachments[kPipelineNormal];
    blendAttachments[kPipelineMultiply].srcColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
    blendAttachments[kPipelineMultiply].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachments[kPipelineMultiply].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    blendAttachments[kPipelineMultiply].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    blendAttachments[kPipelineMultiply].alphaBlendOp = VK_BLEND_OP_ADD;
    // bm 21：SRC_ALPHA / ONE_MINUS_SRC_ALPHA（含 alpha）
    blendAttachments[kPipelineColor] = blendAttachments[kPipelineNormal];
    blendAttachments[kPipelineColor].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachments[kPipelineColor].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachments[kPipelineColor].alphaBlendOp = VK_BLEND_OP_ADD;

    for (int i = 0; i < 3; i++)
    {
        VkPipelineColorBlendStateCreateInfo colorBlend{};
        colorBlend.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlend.attachmentCount = 1;
        colorBlend.pAttachments = &blendAttachments[i];

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &vertexInput;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisample;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlend;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = meshPipelineLayout_;
        pipelineInfo.renderPass = meshClearPass_;
        pipelineInfo.subpass = 0;
        if (!CheckVkResult(vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                                     &meshPipelines_[i]), "CreateGraphicsPipelines"))
        {
            vkDestroyShaderModule(device_, vertModule, nullptr);
            vkDestroyShaderModule(device_, fragModule, nullptr);
            return false;
        }
    }
    vkDestroyShaderModule(device_, vertModule, nullptr);
    vkDestroyShaderModule(device_, fragModule, nullptr);
    return true;
}

//---------------------------------------------------------------------------
// 目标（离屏）
//---------------------------------------------------------------------------
VulkanRenderBackend::Target* VulkanRenderBackend::FindTarget(void* handle) const
{
    for (Target* t : targets_)
    {
        if (t == handle)
            return t;
    }
    return nullptr;
}

void* VulkanRenderBackend::CreateTarget(int width, int height)
{
    if (!EnsureMeshResources() || width <= 0 || height <= 0)
        return nullptr;

    Target* target = new Target();
    target->width = width;
    target->height = height;

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.extent = {(uint32_t)width, (uint32_t)height, 1};
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    if (!CheckVkResult(vkCreateImage(device_, &imageInfo, nullptr, &target->image), "CreateTargetImage"))
    {
        delete target;
        return nullptr;
    }

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(device_, target->image, &memReq);
    uint32_t memoryType = FindHostVisibleMemory(physicalDevice_, memReq, false);
    if (memoryType == 0xFFFFFFFF)
        memoryType = 0;
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryType;
    if (!CheckVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &target->memory), "AllocateTargetMemory"))
    {
        vkDestroyImage(device_, target->image, nullptr);
        delete target;
        return nullptr;
    }
    vkBindImageMemory(device_, target->image, target->memory, 0);

    // 一次性转换到 GENERAL（之后绘制/采样/回读均无需再转换）
    VkCommandBufferAllocateInfo cmdAlloc{};
    cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdAlloc.commandPool = commandPool_;
    cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdAlloc.commandBufferCount = 1;
    VkCommandBuffer cmd;
    if (!CheckVkResult(vkAllocateCommandBuffers(device_, &cmdAlloc, &cmd), "AllocateBarrierCmd"))
    {
        vkFreeMemory(device_, target->memory, nullptr);
        vkDestroyImage(device_, target->image, nullptr);
        delete target;
        return nullptr;
    }
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    vkEndCommandBuffer(cmd);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    vkQueueSubmit(graphicsQueue_, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue_);
    vkFreeCommandBuffers(device_, commandPool_, 1, &cmd);

    // 视图
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = target->image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.layerCount = 1;
    if (!CheckVkResult(vkCreateImageView(device_, &viewInfo, nullptr, &target->view), "CreateTargetView"))
    {
        DestroyTarget(target);
        return nullptr;
    }

    // 帧缓冲（meshClearPass_ 与 meshLoadPass_ 兼容，可共用）
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = meshClearPass_;
    fbInfo.attachmentCount = 1;
    fbInfo.pAttachments = &target->view;
    fbInfo.width = (uint32_t)width;
    fbInfo.height = (uint32_t)height;
    fbInfo.layers = 1;
    if (!CheckVkResult(vkCreateFramebuffer(device_, &fbInfo, nullptr, &target->framebuffer), "CreateTargetFramebuffer"))
    {
        DestroyTarget(target);
        return nullptr;
    }

    // 蒙版描述符（set1：作为蒙版采样）
    VkDescriptorSetAllocateInfo descAlloc{};
    descAlloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descAlloc.descriptorPool = descriptorPool_;
    descAlloc.descriptorSetCount = 1;
    descAlloc.pSetLayouts = &maskSetLayout_;
    if (!CheckVkResult(vkAllocateDescriptorSets(device_, &descAlloc, &target->maskSet), "AllocateMaskSet"))
    {
        DestroyTarget(target);
        return nullptr;
    }
    VkDescriptorImageInfo imageInfoDesc{};
    imageInfoDesc.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    imageInfoDesc.imageView = target->view;
    imageInfoDesc.sampler = sampler_;
    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = target->maskSet;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &imageInfoDesc;
    vkUpdateDescriptorSets(device_, 1, &write, 0, nullptr);

    targets_.push_back(target);
    return target;
}

void VulkanRenderBackend::DestroyTarget(void* handle)
{
    Target* target = FindTarget(handle);
    if (!target)
        return;
    if (!meshReady_)
    {
        // 网格资源未初始化时仅回收对象
        for (size_t i = 0; i < targets_.size(); i++)
        {
            if (targets_[i] == target)
            {
                targets_.erase(targets_.begin() + i);
                break;
            }
        }
        delete target;
        return;
    }
    vkDeviceWaitIdle(device_);
    if (currentTarget_ == target)
    {
        passActive_ = false;
        currentTarget_ = nullptr;
    }
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
    if (target->maskSet)
        vkFreeDescriptorSets(device_, descriptorPool_, 1, &target->maskSet);
    if (target->framebuffer)
        vkDestroyFramebuffer(device_, target->framebuffer, nullptr);
    if (target->view)
        vkDestroyImageView(device_, target->view, nullptr);
    if (target->memory)
        vkFreeMemory(device_, target->memory, nullptr);
    if (target->image)
        vkDestroyImage(device_, target->image, nullptr);
    delete target;
}

void VulkanRenderBackend::SetTarget(void* handle)
{
    EndPass();
    currentTarget_ = FindTarget(handle);
    passClear_ = false;
}

void VulkanRenderBackend::ClearTarget(bool clearColor)
{
    // 软件后端语义：clearColor=false 仅清深度（Vulkan 2D 无深度附件）→ 不清屏
    passClear_ = clearColor;
}

//---------------------------------------------------------------------------
// 命令录制（2D 网格）
//---------------------------------------------------------------------------
bool VulkanRenderBackend::BeginPass(Target* target, bool clear)
{
    if (!target)
        return false;
    if (!commandActive_)
    {
        vkResetCommandBuffer(meshCommandBuffer_, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (!CheckVkResult(vkBeginCommandBuffer(meshCommandBuffer_, &beginInfo), "BeginCommandBuffer"))
            return false;
        commandActive_ = true;
    }

    VkClearValue clearValue = {{{0.0f, 0.0f, 0.0f, 0.0f}}};
    VkRenderPassBeginInfo rpBegin{};
    rpBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpBegin.renderPass = clear ? meshClearPass_ : meshLoadPass_;
    rpBegin.framebuffer = target->framebuffer;
    rpBegin.renderArea.offset = {0, 0};
    rpBegin.renderArea.extent = {(uint32_t)target->width, (uint32_t)target->height};
    rpBegin.clearValueCount = 1;
    rpBegin.pClearValues = &clearValue;
    vkCmdBeginRenderPass(meshCommandBuffer_, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float)target->width;
    viewport.height = (float)target->height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(meshCommandBuffer_, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)target->width, (uint32_t)target->height};
    vkCmdSetScissor(meshCommandBuffer_, 0, 1, &scissor);

    passActive_ = true;
    return true;
}

void VulkanRenderBackend::EndPass()
{
    if (passActive_ && commandActive_)
        vkCmdEndRenderPass(meshCommandBuffer_);
    passActive_ = false;
}

bool VulkanRenderBackend::EnsureStaging(size_t bytes)
{
    if (stagingSize_ >= bytes)
        return true;
    if (stagingBuffer_)
    {
        vkDestroyBuffer(device_, stagingBuffer_, nullptr);
        vkFreeMemory(device_, stagingMemory_, nullptr);
        stagingBuffer_ = VK_NULL_HANDLE;
        stagingMemory_ = VK_NULL_HANDLE;
        stagingMapped_ = nullptr;
        stagingSize_ = 0;
    }
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bytes;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (!CheckVkResult(vkCreateBuffer(device_, &bufferInfo, nullptr, &stagingBuffer_), "CreateStagingBuffer"))
        return false;
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device_, stagingBuffer_, &memReq);
    uint32_t memoryType = FindHostVisibleMemory(physicalDevice_, memReq, true);
    if (memoryType == 0xFFFFFFFF)
        return false;
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = memoryType;
    if (!CheckVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &stagingMemory_), "AllocateStagingMemory"))
        return false;
    vkBindBufferMemory(device_, stagingBuffer_, stagingMemory_, 0);
    vkMapMemory(device_, stagingMemory_, 0, memReq.size, 0, (void**)&stagingMapped_);
    stagingSize_ = bytes;
    return true;
}

bool VulkanRenderBackend::EnsureVertexBuffers(size_t vertexBytes, size_t indexBytes)
{
    auto ensureBuffer = [&](VkBuffer& buffer, VkDeviceMemory& memory, size_t& capacity, size_t bytes,
                            VkBufferUsageFlags usage) -> bool {
        if (capacity >= bytes && buffer)
            return true;
        if (buffer)
        {
            vkDestroyBuffer(device_, buffer, nullptr);
            vkFreeMemory(device_, memory, nullptr);
            buffer = VK_NULL_HANDLE;
            memory = VK_NULL_HANDLE;
            capacity = 0;
        }
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bytes;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        if (!CheckVkResult(vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer), "CreateMeshBuffer"))
            return false;
        VkMemoryRequirements memReq;
        vkGetBufferMemoryRequirements(device_, buffer, &memReq);
        uint32_t memoryType = FindHostVisibleMemory(physicalDevice_, memReq, true);
        if (memoryType == 0xFFFFFFFF)
            return false;
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memReq.size;
        allocInfo.memoryTypeIndex = memoryType;
        if (!CheckVkResult(vkAllocateMemory(device_, &allocInfo, nullptr, &memory), "AllocateMeshMemory"))
            return false;
        vkBindBufferMemory(device_, buffer, memory, 0);
        capacity = bytes;
        return true;
    };
    if (!ensureBuffer(meshVertexBuffer_, meshVertexMemory_, meshVertexCapacity_, vertexBytes,
                      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) ||
        !ensureBuffer(meshIndexBuffer_, meshIndexMemory_, meshIndexCapacity_, indexBytes,
                      VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        return false;
    if (!meshVertexMapped_)
        vkMapMemory(device_, meshVertexMemory_, 0, meshVertexCapacity_, 0, (void**)&meshVertexMapped_);
    if (!meshIndexMapped_)
        vkMapMemory(device_, meshIndexMemory_, 0, meshIndexCapacity_, 0, (void**)&meshIndexMapped_);
    return true;
}

//---------------------------------------------------------------------------
// 一般贴图：与窗口贴图同一实现
//---------------------------------------------------------------------------
void* VulkanRenderBackend::CreateTexture(int width, int height)
{
    if (!EnsureMeshResources())
        return nullptr;
    return CreateTextureInternal(width, height);
}

void VulkanRenderBackend::UpdateTexture(void* handle, const uint8_t* pixels, int width, int height, int pitch)
{
    UpdateTextureInternal(FindTexture(handle), pixels, width, height, pitch);
}

void VulkanRenderBackend::DestroyTexture(void* handle)
{
    DestroyTextureInternal(FindTexture(handle));
}

//---------------------------------------------------------------------------
// 绘制状态与网格绘制
//---------------------------------------------------------------------------
void VulkanRenderBackend::SetMask(void* handle)
{
    maskTarget_ = FindTarget(handle);
}

void VulkanRenderBackend::SetBlendMode(int mode, const float* uniformColor)
{
    blendMode_ = mode;
    skipDraw_ = (mode == 6);
    enableColor_ = false;
    if (mode == 21 && uniformColor)
    {
        enableColor_ = true;
        std::memcpy(uniformColor_, uniformColor, sizeof(uniformColor_));
    }
}

void VulkanRenderBackend::DrawMesh(const float* vertices,
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
    if (!EnsureMeshResources())
        return;

    if (!passActive_)
    {
        if (!BeginPass(currentTarget_, passClear_))
            return;
    }

    // 顶点/索引缓冲（主机可见，录制期间写入；提交发生在 LockTarget，串行安全）
    size_t vertexBytes = (size_t)vertexCount * 4 * sizeof(float);
    size_t indexBytes = (size_t)indexCount * sizeof(uint16_t);
    if (!EnsureVertexBuffers(vertexBytes, indexBytes))
        return;

    int pipelineIndex = kPipelineNormal;
    switch (blendMode_)
    {
        case 1:
        case 4:
            pipelineIndex = kPipelineMultiply;
            break;
        case 21:
            pipelineIndex = kPipelineColor;
            break;
        default:
            pipelineIndex = kPipelineNormal;
            break;
    }
    vkCmdBindPipeline(meshCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelines_[pipelineIndex]);

    std::memcpy(meshVertexMapped_, vertices, vertexBytes);
    std::memcpy(meshIndexMapped_, indices, indexBytes);

    VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(meshCommandBuffer_, 0, 1, &meshVertexBuffer_, &offset);
    vkCmdBindIndexBuffer(meshCommandBuffer_, meshIndexBuffer_, 0, VK_INDEX_TYPE_UINT16);

    // 描述符：set0 = 纹理；set1 = 蒙版（无蒙版时用全白 1x1）
    VkDescriptorSet sets[2] = {texture->set,
                               maskTarget_ ? maskTarget_->maskSet : blankMask_->set};
    vkCmdBindDescriptorSets(meshCommandBuffer_, VK_PIPELINE_BIND_POINT_GRAPHICS, meshPipelineLayout_, 0, 2,
                            sets, 0, nullptr);

    MeshPushConstants pc{};
    std::memcpy(pc.uniformColor, uniformColor_, sizeof(uniformColor_));
    pc.viewportX = (float)currentTarget_->width;
    pc.viewportY = (float)currentTarget_->height;
    pc.enableMask = maskTarget_ ? 1.0f : 0.0f;
    pc.enableColor = enableColor_ ? 1.0f : 0.0f;
    pc.opa = opacity;
    vkCmdPushConstants(meshCommandBuffer_, meshPipelineLayout_, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(pc),
                       &pc);

    vkCmdDrawIndexed(meshCommandBuffer_, (uint32_t)indexCount, 1, 0, 0, 0);
}

//---------------------------------------------------------------------------
// 回读
//---------------------------------------------------------------------------
uint8_t* VulkanRenderBackend::LockTarget(void* handle, int& pitch)
{
    Target* target = FindTarget(handle);
    if (!target || !EnsureMeshResources())
        return nullptr;

    EndPass();

    // 渲染 → 回读（GENERAL 布局免转换，仅需内存屏障）
    if (!commandActive_)
    {
        vkResetCommandBuffer(meshCommandBuffer_, 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        if (!CheckVkResult(vkBeginCommandBuffer(meshCommandBuffer_, &beginInfo), "BeginCommandBuffer"))
            return nullptr;
        commandActive_ = true;
    }
    size_t bytes = (size_t)target->width * target->height * 4;
    if (!EnsureStaging(bytes))
        return nullptr;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = target->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    vkCmdPipelineBarrier(meshCommandBuffer_, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    VkBufferImageCopy region{};
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent = {(uint32_t)target->width, (uint32_t)target->height, 1};
    vkCmdCopyImageToBuffer(meshCommandBuffer_, target->image, VK_IMAGE_LAYOUT_GENERAL, stagingBuffer_, 1,
                           &region);
    if (!CheckVkResult(vkEndCommandBuffer(meshCommandBuffer_), "EndCommandBuffer"))
        return nullptr;
    commandActive_ = false;

    vkResetFences(device_, 1, &meshFence_);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &meshCommandBuffer_;
    if (!CheckVkResult(vkQueueSubmit(graphicsQueue_, 1, &submitInfo, meshFence_), "QueueSubmit"))
        return nullptr;
    vkWaitForFences(device_, 1, &meshFence_, VK_TRUE, UINT64_MAX);

    pitch = target->width * 4;
    return stagingMapped_;
}

void VulkanRenderBackend::UnlockTarget(void* handle)
{
    (void)handle;
}

//---------------------------------------------------------------------------
// 清理
//---------------------------------------------------------------------------
void VulkanRenderBackend::Shutdown()
{
    if (!initialized_ && instance_ == VK_NULL_HANDLE)
        return;
    if (device_ != VK_NULL_HANDLE)
        vkDeviceWaitIdle(device_);

    // 2D 网格目标
    for (Target* t : targets_)
    {
        if (t->maskSet)
            vkFreeDescriptorSets(device_, descriptorPool_, 1, &t->maskSet);
        if (t->framebuffer)
            vkDestroyFramebuffer(device_, t->framebuffer, nullptr);
        if (t->view)
            vkDestroyImageView(device_, t->view, nullptr);
        if (t->memory)
            vkFreeMemory(device_, t->memory, nullptr);
        if (t->image)
            vkDestroyImage(device_, t->image, nullptr);
        delete t;
    }
    targets_.clear();

    // 贴图（窗口贴图与一般贴图共用同一列表）
    for (Texture* tex : textures_)
    {
        if (tex->set)
            vkFreeDescriptorSets(device_, descriptorPool_, 1, &tex->set);
        if (tex->view)
            vkDestroyImageView(device_, tex->view, nullptr);
        if (tex->memory)
            vkFreeMemory(device_, tex->memory, nullptr);
        if (tex->image)
            vkDestroyImage(device_, tex->image, nullptr);
        delete tex;
    }
    textures_.clear();
    blankMask_ = nullptr;

    if (device_ != VK_NULL_HANDLE)
    {
        // 2D 网格资源
        if (stagingBuffer_)
            vkDestroyBuffer(device_, stagingBuffer_, nullptr);
        if (stagingMemory_)
            vkFreeMemory(device_, stagingMemory_, nullptr);
        stagingBuffer_ = VK_NULL_HANDLE;
        stagingMemory_ = VK_NULL_HANDLE;
        stagingMapped_ = nullptr;
        stagingSize_ = 0;
        if (meshIndexBuffer_)
            vkDestroyBuffer(device_, meshIndexBuffer_, nullptr);
        if (meshIndexMemory_)
            vkFreeMemory(device_, meshIndexMemory_, nullptr);
        meshIndexBuffer_ = VK_NULL_HANDLE;
        meshIndexMemory_ = VK_NULL_HANDLE;
        meshIndexCapacity_ = 0;
        meshIndexMapped_ = nullptr;
        if (meshVertexBuffer_)
            vkDestroyBuffer(device_, meshVertexBuffer_, nullptr);
        if (meshVertexMemory_)
            vkFreeMemory(device_, meshVertexMemory_, nullptr);
        meshVertexBuffer_ = VK_NULL_HANDLE;
        meshVertexMemory_ = VK_NULL_HANDLE;
        meshVertexCapacity_ = 0;
        meshVertexMapped_ = nullptr;
        if (meshFence_)
            vkDestroyFence(device_, meshFence_, nullptr);
        meshFence_ = VK_NULL_HANDLE;
        for (int i = 0; i < 3; i++)
        {
            if (meshPipelines_[i])
                vkDestroyPipeline(device_, meshPipelines_[i], nullptr);
            meshPipelines_[i] = VK_NULL_HANDLE;
        }
        if (meshPipelineLayout_)
            vkDestroyPipelineLayout(device_, meshPipelineLayout_, nullptr);
        meshPipelineLayout_ = VK_NULL_HANDLE;
        if (meshLoadPass_)
            vkDestroyRenderPass(device_, meshLoadPass_, nullptr);
        if (meshClearPass_)
            vkDestroyRenderPass(device_, meshClearPass_, nullptr);
        meshClearPass_ = meshLoadPass_ = VK_NULL_HANDLE;
        if (maskSetLayout_)
            vkDestroyDescriptorSetLayout(device_, maskSetLayout_, nullptr);
        maskSetLayout_ = VK_NULL_HANDLE;

        // 窗口资源
        if (commandPool_)
            vkDestroyCommandPool(device_, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
        frameCommandBuffer_ = VK_NULL_HANDLE;
        meshCommandBuffer_ = VK_NULL_HANDLE;
        if (frameFence_)
            vkDestroyFence(device_, frameFence_, nullptr);
        if (imageReady_)
            vkDestroySemaphore(device_, imageReady_, nullptr);
        if (renderFinished_)
            vkDestroySemaphore(device_, renderFinished_, nullptr);
        if (sampler_)
            vkDestroySampler(device_, sampler_, nullptr);
        if (windowVertexBuffer_)
            vkDestroyBuffer(device_, windowVertexBuffer_, nullptr);
        if (windowVertexMemory_)
            vkFreeMemory(device_, windowVertexMemory_, nullptr);
        if (windowIndexBuffer_)
            vkDestroyBuffer(device_, windowIndexBuffer_, nullptr);
        if (windowIndexMemory_)
            vkFreeMemory(device_, windowIndexMemory_, nullptr);
        if (descriptorPool_)
            vkDestroyDescriptorPool(device_, descriptorPool_, nullptr);
        if (textureSetLayout_)
            vkDestroyDescriptorSetLayout(device_, textureSetLayout_, nullptr);
        if (windowPipeline_)
            vkDestroyPipeline(device_, windowPipeline_, nullptr);
        if (windowPipelineLayout_)
            vkDestroyPipelineLayout(device_, windowPipelineLayout_, nullptr);
        if (windowRenderPass_)
            vkDestroyRenderPass(device_, windowRenderPass_, nullptr);
        DestroySwapchain();
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }
    if (surface_)
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }
    if (instance_)
    {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
    initialized_ = false;
    meshReady_ = false;
    currentTarget_ = nullptr;
    maskTarget_ = nullptr;
    passActive_ = false;
}

//---------------------------------------------------------------------------
// 工厂与注册
//---------------------------------------------------------------------------
iTVPRenderBackend* CreateVulkanRenderBackend(VkInstance _instance, VkSurfaceKHR _surface)
{
    VulkanRenderBackend* backend = new VulkanRenderBackend(_instance, _surface);
    if (!backend->Initialize())
    {
        delete backend;
        return nullptr;
    }
    return backend;
}

bool VulkanRenderBackendAvailable()
{
    // 编译进来即可用。真正的 loader/驱动探测在 Initialize() 中完成
    // （SDL_Vulkan_LoadLibrary 需在 SDL_Init 视频子系统之后调用，
    //  而参数解析阶段的 probe 运行在 SDL_Init 之前）。
    return true;
}

namespace
{
struct VulkanRenderBackendAutoRegister
{
    VulkanRenderBackendAutoRegister()
    {
        TVPRenderBackendDesc desc;
        desc.name = "vulkan";
        desc.description = "Vulkan 1.0 (via SDL3 surface)";
        desc.probe = VulkanRenderBackendAvailable;
        desc.create = nullptr;
        TVPRegisterRenderBackend(desc);
    }
} gVulkanRenderBackendAutoRegister;
} // namespace

} // namespace krkrsdl3

#endif // _KRKRSDL3_USE_SDL3 && _KRKRSDL3_USE_VULKAN
