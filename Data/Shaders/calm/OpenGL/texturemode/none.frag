#version 400 core

uniform sampler2D ##TEXTURE_UNIT##;

vec4 textureMode_##TEXTURE_UNIT##(vec4 c, vec2 uv) {
    return c;
}
