
# Texture Mode Shaders

These shaders are templates, defining a function which blends a texture's data into a previous colour.
That colour may be the vertex colour, or a colour calculated by one or more prior textures.

```c
// All shaders should match calm display requirements - GL 4 or higher
#version 400 core

// Uniforms for a given texture unit
uniform bool ##TEXTURE_UNIT##Enabled;
uniform sampler2D ##TEXTURE_UNIT##;

// Sample function
vec4 textureMode_##TEXTURE_UNIT##(vec4 c, vec2 uv) {
    if( ##TEXTURE_UNIT##Enabled == false ) {
        return c;
    }
    vec4 t = texture(##TEXTURE_UNIT##, uv);
    // In this example (modulate) - Multiply previous colour by texture
    vec4 fragColor = c * t;
    return fragColor;
}
```
