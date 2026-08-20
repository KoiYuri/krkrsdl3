#version 450
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
layout(push_constant) uniform PushConstants {
    vec2 position; // 目标矩形左上角（像素）
    vec2 size;     // 目标矩形尺寸（像素）
    vec2 viewport; // 窗口尺寸（像素）
} pc;
layout(location = 0) out vec2 TexCoord;
void main()
{
    vec2 pixelPos = pc.position + pc.size * aPos;
    vec2 ndcPos = pixelPos * 2.0 / pc.viewport - 1.0;
    // Vulkan 的 NDC Y 轴向下（-1 顶部，+1 底部），与 OpenGL 相反；
    // 翻转 Y 使最终上屏方向与 GL/软渲染后端一致（含纹理方向）。
    ndcPos.y = -ndcPos.y;
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
