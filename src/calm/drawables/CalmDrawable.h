#pragma once

#include "calm/drawables/CalmRenderState.h"
#include "RageTypes.h"

/**
 * Drawables supported by all (or some?) of the calm Display classes
 * - A drawable represents one or more draw commands and gpu resources
 * - Ideally, drawables may be batched together, but that's up to the underlying display
 * - Each renderable is bound to an Actor type e.g. a SpriteRenderable covers everything that Sprite could want
 * - Actors own one or more drawables
 * - During the (old) draw path actors update the parameters of their Drawables
 *   - Ideally, only parameters such as location / matrices - Things that don't modify gpu resources such as vertices
 * - The calm draw path collates all drawables into a single list
 * - The drawable is passed to the display
 * - The display is free to execute all drawable's commands in sequence, or re-batch as desired
 * 
 * Depth is an important concept for SM, since the bulk of rendering is 2D.
 * In the old path this is handled by enabling/disabling depth tests, and fully clearing the depth
 * buffer between each arrow being rendered (yes, thousands of gl calls).
 * Under calm, that concept maps to depth slices and draw passes:
 * - A draw pass starts with a cleared depth buffer. All further draw commands are over previous data
 * - 2D elements are assigned a depth index
 *   - When executing a 2D draw pass, each draw call / primitive has a depth slice, translated to an actual z value
 *   - When switching to 3D, a new draw pass is needed - Clearing the depth buffer
 * - 3D elements are assigned depth, as a normal renderer would
 *   - For elements such as arrows, depth may be slightly offset forward to prevent z fighting
 *   - Or, mimicking the old path, arrows are drawn in sequence with depth test disabled - But would prevent batching?
 * 
 * The Drawable class hierarchy provides a number of concepts
 * - Base classes are renderer-agnostic (not specific to opengl, vulkan, or anything)
 * - A factory instance is provided by the display to instantiate renderables
 * - Implementations of supported renderables are provided by the display
 * - If a renderable isn't supported, the factory method will fail
 *   - But all displays should support all renderables in some form
 *   - This should only happen where an Actor uses a highly-specialised drawable,
 *     such as specifically wanting an instanced render of 500,000 arrows at a time
 */

namespace calm {
    class Display;

    class Drawable {
        public:
            ~Drawable();

            void validate(Display* display);
            void draw(Display* display);
            void invalidate(Display* display);

            const bool& valid() const { return mValid; }

            // Common state parameters for the drawable
            // Populated by the latest state of various stacks
            // and globals in engine, stored until evaluated
            // by the calm::Display.
            float modelViewMatrix[4][4];
            float projectionMatrix[4][4];
            float textureMatrix[4][4];

            // Global render state for the draw
            RenderState renderState;

            /*
             * Rendering modes
             *
             * The Rage display setup is built around a number of
             * rendering modes, dictating how textures and such
             * are drawn. These relate to the OpenGL 1.x functionality
             * e.g. TextureBlendMode::Modulate.
             * 
             * For calm drawables the implementation of these is down
             * to the implementation, but likely a parameterised
             * shader to provide certain modes (or preprocessed).
             * 
             * Note: Rage may have up to 4 textures active
             * (RageDisplay::TextureUnit), and these each have
             * a different render mode - The calm display must
             * respect these to be visually correct, at least
             * until an Actor's draw path can be migrated into
             * a dedicated drawable.
             * 
             * The intention is that multi-mode Actors such as
             * Sprite (body, fading, shadow, glow) are migrated
             * into dedicated Drawable types, though note that this
             * will restrict the flexibility of calm.
             * e.g. Under Rage, a modified Sprite could decide to use
             * a different texture mode, and RageDisplay would work fine,
             * but under a DrawableSprite only the modes currently needed
             * by Sprite would be supported.
             * 
             * However - while emulating the full set of fixed-function modes
             * in a modern API is possible, it _will_ result in worse performance
             * than just using the 25+ year old approach - In the best case,
             * we cannot compete with NVidia's emulation of OpenGL 1.x.
             * 
             * So all that said, some rendering modes are defined on Drawable,
             * and should be respected by implementations.
             */
            // TODO: Currently no render modes exist, and we don't populate these
            //       during the draw loop - I'm moving the whole of sprite rendering
            //       into DrawableSprite, and writing specific shaders for it,
            //       rather than fail at emulating the OpenGL 1.x pipeline a second time.

            // TODO: Draw modes and depth slicing
            //       (So we can batch multiple draws together later)
            // unsigned int depthSlice = 0;

            // TODO: Texture matrix scale and friends
            // TODO: All the other global stuff like effect and blend modes
            // - Some of these will affect all drawables
            // - Others are actually only used for certain draws anyway,
            //   so should be part of the more specific drawables.
            // - Unless we can have one megashader, but that was fairly
            //   slow in the GL4 prototype.

        protected:
            Drawable();

            virtual bool doValidate(Display* display) = 0;
            virtual void doDraw(Display* display) = 0;
            virtual void doInvalidate(Display* display) = 0;

            bool mValid = false;            
    };
}
