#version 400 core

uniform bool ##TEXTURE_UNIT##Enabled;
uniform sampler2D ##TEXTURE_UNIT##;

vec4 textureMode_##TEXTURE_UNIT##(vec4 c, vec2 uv) {
    if( ##TEXTURE_UNIT##Enabled == false ) {
        return c;
    }
    vec4 t = texture(##TEXTURE_UNIT##, uv);
    vec4 fragColor = c * t;
    return fragColor;
}
