
# Calm Display

This is the latest attempt by ITGaz (geefr) to rework the rendering code.

Please don't use it outside ITGMania, specifically the 'clarity' branch of https://github.com/geefr/ITGMania

At least until it's done cooking, the contents of this folder is licenced as GPL3

# Status

Very early prototype. Currently CalmDisplayOpenGL can render a single clear colour to the window, and that's it.

# Concepts

While my previous attempt at an OpenGL 4.x RageDisplay directly implemented RageDisplay, it had
to map the engine's draw path (built on OpenGL 1.x) over to OpenGL 4.x. Essenitally emulating the
ancient GL functions using modern tech.

This worked to a degree, but meant that I was trying to beat NVidia at their own game - I got good
performance, but not better than throwing the old OpenGL calls at the driver. Similar results on
Intel and AMD, with their respective quirks.

Calm is a fresh take, including modifications to Rage / the Actor's draw path.
New capability is kept somewhat distinct to allow new concepts to be used, with the concept of
Drawables / Draw commands used for the transation - The Actor's draw path should provide
a set of persistent Drawables, which act as the scene graph for CalmDisplay to render.

Initially I'm only implementing an OpenGL path, and probably with quite limited performance e.g.
not including instanced rendering or complex batching mechanisms, just mapping the Actors over
to a set of drawables, and handling the challenges of depth slice / z management.

On the stepmania / rage side of things, the Calm display is exposed as `DISPLAY2`. Typical logic
for an actor is then to create drawables `if(DISPLAY2)`, and otherwise execute the current
draw path against `DISPLAY`.

# Stepmania Renderer Notes

A summary of features as used by the engine, and as implemented for RageDisplay / Calm Display

## Window & Context Init

In Stepmania.cpp:CreateDisplay (or now CreateDisplay2) a renderer is created, and init is called.
Calls LowLevelWindow::Create - provides a graphics context and such - Window extended in this branch to provide GL 4.6 Core Profile, or the newest possible core profile.

(That's right - No fixed function pipeline, compatibility mode is banned..We'll tackle integration of ancient glsl shaders later perhaps)

## Matrix Stack

The matrix stack is managed by RageDisplay (TODO: This is a bad idea, it needs to be split out), as g_WorldStack and friends.

At any point in the draw function, the latest matrices can be pulled off the stack via
```C++
RageMatrix projection;
RageMatrixMultiply(&projection, RageMatrices::GetCentering(), RageMatrices::GetProjectionTop());
std::memcpy(d->projectionMatrix, static_cast<const float*>(projection), 16 * sizeof(float));
RageMatrix modelView;
RageMatrixMultiply(&modelView, RageMatrices::GetViewTop(), RageMatrices::GetWorldTop());
std::memcpy(d->modelViewMatrix, static_cast<const float*>(modelView), 16 * sizeof(float));
RageMatrix texture = *RageMatrices::GetTextureTop();
std::memcpy(d->textureMatrix, static_cast<const float*>(texture), 16 * sizeof(float));
```

I'm not sure what 'centering' is, but the projection matrix is as you'd expect
* TODO: What's the depth range on that?

And the modelview is too

The texture matrix is a bit of a strange one. I'm not entirely confident on it but it's applied in a similar way to a model matrix, just scaling the texture coordinates for whatever's being rendered.

In addition, shaders have a paremeter called 'texturematrixscale', which is passed through the SpriteVertex (RageTypes.h). In most cases it's constant for all vertices of the draw, but it's a very important one.

Texture matrix scale (TODO: Check this) multiplies against the translation parameter of the texture matrix, controlling animation and similar indexing into a texture.

Old shader: 
```c
vec4 multiplied_tex_coord = gl_TextureMatrix[0] * gl_MultiTexCoord0;
gl_TexCoord[0] = (multiplied_tex_coord * TextureMatrixScale) +
                 (gl_MultiTexCoord0 * (vec4(1)-TextureMatrixScale));
```

Or translated to a less-mystic version (TODO: Double check this - was correct visually but forgot how it works):
```c
mat4 tMat = textureMat;
if( enableTextureMatrixScale )
{
    tMat[3][0] *= textureMatrixScale.x;
    tMat[3][1] *= textureMatrixScale.y;
}
```

Noted uses:
* RageCompiledGeometry - 3D models and noteskins have m_bNeedsTextureMatrixScale
* Data/Shaders/GLSL/Texture Matrix Scale.vert
* I think that's it - Should be (0,0) or disabled for everything else, isn't used by other shaders (But I'm not sure which shaders get combined by RageDisplay_GL)

## Render Modes

These are similar global parameters, activated through RageDisplay and then left enabled unless the caller reverts the change.

TODO: No global stack exists for these? Mapping over to drawables will need such a thing, to cache whatever the settings were when the draw was compiled

### Texture Modes

TODO: Modulate is the default right!??

Defined in RageTypes.h

These are confusing, but map through to texture mapping in a fragment shader, for modern GL.
When rendering an Actor, any number of shaders can be set (up to 4 though), and these seem
to get applied in order - Whatever it is the OpenGL 1.x pipeline does, or something visually equivalent to the old render path.

These are set for a specific textures i.e. if a draw used 4 textures, they could all have different texture modes.

Modulate: "Affects one texture stage. Texture is modulated with the diffuse color."
* For rgba textures: `fragColor = inColor * texture;`
* For rgb textures: `fragColor = inColor.rgb * texture.rgb; fragColor.a = 1.0;`

Modulate in GL1 path:
```c++
glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
```

Glow: "Affects one texture stage. Color is replaced with white, leaving alpha. Used with BLEND_ADD to add glow."
* TODO: Blend modes just don't exist in modern GL
* `fragColor = vec4(inColor.rgb, (inColor.a * texture.a)));`

```c++
// the below function is glowmode,brighten:
if (!GLEW_ARB_texture_env_combine && !GLEW_EXT_texture_env_combine)
{
    /* This is changing blend state, instead of texture state, which
        * isn't great, but it's better than doing nothing. */
    glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    return;
}

// and this is glowmode,whiten:
// Source color is the diffuse color only:
glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_EXT);
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_COMBINE_RGB_EXT), GL_REPLACE);
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_SOURCE0_RGB_EXT), GL_PRIMARY_COLOR_EXT);

// Source alpha is texture alpha * diffuse alpha:
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_COMBINE_ALPHA_EXT), GL_MODULATE);
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_OPERAND0_ALPHA_EXT), GL_SRC_ALPHA);
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_SOURCE0_ALPHA_EXT), GL_PRIMARY_COLOR_EXT);
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_OPERAND1_ALPHA_EXT), GL_SRC_ALPHA);
glTexEnvi(GL_TEXTURE_ENV, GLenum(GL_SOURCE1_ALPHA_EXT), GL_TEXTURE);
```

Add: "Affects one texture stage. Color is added to the previous texture stage."
* TODO: `fragColor = vec4(inColor + texture);`

### Blend Modes

TODO: Document these - Either it's alpha blending modes in GL, or shader settings

Modulate:
```c++
glBlendFunc( GL_ZERO, GL_SRC_COLOR );
```

Add:
```c++
glBlendFunc( GL_SRC_ALPHA, GL_ONE );
```

### Effect Modes

TODO: Document these - Either it's alpha blending modes in GL, or shader settings

## Depth

TODO: What the heck?

```c++
void Actor::SetGlobalRenderStates()
{
	// set Actor-defined render states
	if( !g_bShowMasks.Get() || m_BlendMode != BLEND_NO_EFFECT )
		DISPLAY->SetBlendMode( m_BlendMode );
	DISPLAY->SetZWrite( m_bZWrite );
	DISPLAY->SetZTestMode( m_ZTestMode );

	// BLEND_NO_EFFECT is used to draw masks to the Z-buffer, which always wants
	// Z-bias enabled.
	if( m_fZBias == 0 && m_BlendMode == BLEND_NO_EFFECT )
		DISPLAY->SetZBias( 1.0f );
	else
		DISPLAY->SetZBias( m_fZBias );

	if( m_bClearZBuffer )
		DISPLAY->ClearZBuffer();
	DISPLAY->SetCullMode( m_CullMode );
}
```

## Actors

### Sprite

Sprite rendering is of course complex, since `Sprite` is pretty much the entire UI - All backgrounds, menus, text, videos; ALL OF THEM!

Sprite rendering like any other Actor has a bunch of matrix logic on the matrix stack, and eventually boils down to a few DrawQuad calls.

RageTypes.h defines a sprite vertex - vertex position, normal, (diffuse) colour, texcoord

Sprite::v holds 4 of those - Scaled and clipped accordingly during Sprite::Draw
* Note: This is likely to change for calm display, since modifying the vertex data itself will be slow under modern OpenGL / other APIs
* TODO: I'm unsure of the order on these
  * Sprite::v and other quad-based comments indicate it's tl, bl, br, tr
  * In various graphics debuggers for the GL4 prototype I noticed that's backwards

Sprite uses effect modes:
* Normal

Sprite uses texture modes:
* Modulate - Shadow and diffuse
* Glow - Glow pass

Animation of a sprite is handled by modifying texture coords on each frame - Sprite::SetCustomTextureCoords

DrawPrimitives:
* If fading edges
  * Calls DrawTexture 5 times for center and 4 sides
  * Each with a modified tweenstate
* If not fading edges
  * Calls DrawTexture once, with m_pTempState
  * TODO: Find out what this is, it's in Actor.h

Notably for CalmDisplay - Those 5 calls to DrawTexture should all use different drawables.
For now, and since Sprite resets all vertex data each draw anyway, a fresh set of drawables
is created during DrawTexture.
* TODO: This is terrible for performance
* TODO: While the logic is confusing, there's no reason the fade stuff couldn't be implemented in shader

DrawTexture
* Does a bunch of weird cropping logic
* Then calls DrawQuad

TODO: Sometimes the sprite's texture isn't set
* Is this valid? Just a coloured quad instead?

#### Sprite Fade

Old render path:

```
Fading a sprite first renders the inside, within the bounds of the FadeDist - e.g.
fade == 0.2 on all sides would trim 0.2 off each side, and render the remaining
center as normal.

Each border region is then rendered, as an alpha-scaled between 0 and 1 across the 
border region.

Sprite::DrawPrimitive - Each if block calculated ts.crop, to render only the
faded region for the draw e.g. top == 0.2 crops to 0.2 * height across the sprite,
before rendering.

If left or right are faded as well as top/bottom, the crop is inset at the edges by the left/right fade
resulting in the corner being discarded entirely e.g. if top+left == 0.4 are set, a cap / quad is not
rendered in the newly-created top-left region:

-------------|----------------
|  missing   | faded top     |
|            |               |
-------------|----------------
| faded left | inside quad   |
|            |               |
-------------|---------------|

To be visually correct, that mising quad would need to be rendered with alpha = 0 on tl, and alpha
matching the top/left fades at br.

(Discussed with teejusb - Not modifying the old codepath, but interesting point for the new one)

If a crop is also specified it's applied first - Fade starts from the cropped edge.
In this case fade distance is still a fraction of the full sprite i.e. croptop(0.25)
and fadetop(0.25) would discard 0.25 from the top, then blend 0.25, finishing the fade
at the center (0.5) of the sprite.

Colour is controlled by bumping the vertex colour from the sprite edge to the inner
border (at alpha == 1), and scaling alpha between 0 and TopAlpha (1, at the inner edge
of the fade). A lot of complex code which results in an alpah ramp across a border region.

If shadow is enabled, fade does not affect shadow alpha, however does chop the corners off,
due to the cropping behaviour, and modification to shadow diffuse colour in DrawTexture.
This results in a faded sprite, but a hard shadow underneath it.

(Seems like another oddity / bug with the old renderer)
```

New / calm render path:

```
Given quirks with corner fades and shadows, seems easier to just re-implement a fade from scratch.
* A crop defines the render area 
* A fade defines alpha ramp for each edge

Ideally both crop and fade would be handled in vertex shader, but the cropping logic is complex.
Includes cropping and custom vertex position coords, all sorts.

TODO: Need to determine x/y coord within quad in fragment shader, with 0-1 corresponding to
cropped region. i.e. similar to texture coords, but always 0 -> 1.

```
