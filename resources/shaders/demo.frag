#version 450

precision mediump float;
layout(binding = 0) uniform highp sampler2D main_tex;
in mediump vec2 uv;
out lowp vec4 outcolor;

void main() {
    outcolor = texture(main_tex, uv);
}