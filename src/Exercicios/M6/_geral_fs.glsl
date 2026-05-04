#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform float uAlpha;
uniform vec4 uColor;
uniform int uUseTexture;

void main() {
    if (uUseTexture == 0) {
        FragColor = uColor;
        return;
    }

    vec4 color = texture(uTexture, TexCoord);
    // Descarta pixels transparentes para sprites e tiles com alpha
    if (color.a <= 0.01) {
        discard;
    }
    FragColor = vec4(color.rgb, color.a * uAlpha);
}
