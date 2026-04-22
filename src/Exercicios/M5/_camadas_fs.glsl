#version 330 core

in vec2 uv;
out vec4 frag_color;

uniform sampler2D sprite;

void main() {
    vec4 tex = texture(sprite, uv);

    if (tex.a < 0.1)
        discard;

    frag_color = tex;
}