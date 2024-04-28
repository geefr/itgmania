vec4 textureMode_##TEXTURE_UNIT##(vec4 c, vec2 uv) {
    if( texture##TEXTURE_UNIT##enabled == false ) {
        return c;
    }
    vec4 t = texture(texture##TEXTURE_UNIT##, uv);
    vec4 fragColor = c * t;
    return fragColor;
}
