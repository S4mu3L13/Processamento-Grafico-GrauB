#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;

out vec2 uv;

uniform mat4 model;

void main() {
    gl_Position = model * vec4(position, 0.0, 1.0);
    uv = texcoord;
}