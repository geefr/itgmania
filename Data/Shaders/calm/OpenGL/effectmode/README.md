
# Effect Mode Shaders

These shaders are not templates (unlike texture modes).

Each effect mode specifies a number of textures that can be displayed - Uniforms should not be present
for unused texture units. If the engine tries to use more textures, either the renderer will silently
discard, or warn (due to a failed lookup of uniform location).

An effect mode overrides any and all texture modes - Either an effect mode is loaded, or 1+ texture modes are.

Effect shaders are written for specific use cases, such as handling YUV422 for video display.

```c
// All shaders should match calm display requirements - GL 4 or higher
#version 400 core

// Uniforms for the supported textures
uniform bool texture0Enabled;
uniform sampler2D texture0;

// TODO: Any additional uniforms - Some are unique to sprites and shouldn't be relied upon!

// Sample function, in the same format as a texture mode
// TODO: If multiple textures are used, that breaks everything right? Try and port colourdodge or something.
vec4 textureMode_texture0(vec4 c, vec2 uv) {
    if( texture0Enabled == false ) {
        return c;
    }
    vec4 t = texture(texture0 uv);
    // TODO: Example of effect mode
    return fragColor;
}
```
