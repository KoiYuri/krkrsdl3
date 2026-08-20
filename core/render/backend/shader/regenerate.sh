#!/usr/bin/env bash
# 重新生成 vulkan_shaders.h（由 GLSL 编译为 SPIR-V 并嵌入 C 数组）
# 依赖：glslc（Vulkan SDK / glslang-tools）与 python3
set -euo pipefail
cd "$(dirname "$0")"

glslc --target-env=vulkan1.0 -fshader-stage=vert vk_quad.vert -o vk_quad.vert.spv
glslc --target-env=vulkan1.0 -fshader-stage=frag vk_quad.frag -o vk_quad.frag.spv
spirv-val --target-env vulkan1.0 vk_quad.vert.spv
spirv-val --target-env vulkan1.0 vk_quad.frag.spv

python3 - <<'PYEOF'
import struct
def to_c_array(path, name):
    data = open(path, 'rb').read()
    words = struct.unpack('<%dI' % (len(data)//4), data)
    lines = []
    for i in range(0, len(words), 8):
        chunk = ', '.join('0x%08x' % w for w in words[i:i+8])
        lines.append('    ' + chunk + ',')
    body = '\n'.join(lines)
    return f"static const uint32_t {name}[] = {{\n{body}\n}};"

vert = to_c_array('vk_quad.vert.spv', 'kVulkanVertSpv')
frag = to_c_array('vk_quad.frag.spv', 'kVulkanFragSpv')
out = f"""// 自动生成：由 GLSL 经 glslc --target-env=vulkan1.0 编译嵌入（重新生成：shader/regenerate.sh）
// 源文件: vk_quad.vert / vk_quad.frag（同目录）
// 顶点着色器：单位四边形 + push constant 变换（与 GL 后端同一套像素坐标语义）
{vert}

// 片元着色器：纹理采样
{frag}
"""
open('vulkan_shaders.h', 'w').write(out)
print("vulkan_shaders.h regenerated")
PYEOF
