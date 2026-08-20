#version 450
layout(location = 0) in vec2 TexCoord;
layout(location = 0) out vec4 FragColor;
layout(set = 0, binding = 0) uniform sampler2D texture1;
void main()
{
    FragColor = texture(texture1, TexCoord);
}
