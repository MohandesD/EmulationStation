#version 450
layout(location=0) in vec4 position;
layout(location=1) in vec4 color;
layout(location=2) in vec2 uv;
layout(location=0) out vec4 vertexColor;
layout(location=1) out vec2 textureUV;
void main() {
    gl_Position=vec4(position.xy, 0.5 * (position.z + position.w), position.w);
    vertexColor=color;
    textureUV=uv;
}
