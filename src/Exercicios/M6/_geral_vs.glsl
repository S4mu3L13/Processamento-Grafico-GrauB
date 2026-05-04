#version 410 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform vec2 uScreenSize;
uniform vec2 uOffset;

void main() {
    // Soma o deslocamento do objeto e converte de coordenada de tela para OpenGL
    vec2 pixel = aPos + uOffset;
    vec2 ndc;
    ndc.x = (pixel.x / uScreenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (pixel.y / uScreenSize.y) * 2.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    TexCoord = aTexCoord;
}
