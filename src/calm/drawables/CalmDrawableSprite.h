#pragma once

#include "calm/drawables/CalmDrawable.h"

#include <array>

namespace calm {
    /**
     * Drawable for sprite / textured quad rendering
     */
    class DrawableSprite : public Drawable {
        public:
            virtual ~DrawableSprite() {}

            // TODO: Global render mode stuff
            // Actor::SetGlobalRenderStates
            // - blendmode - sprite doesn't set one
            // - Zwrite
            // - ZTestMode
            // - ZBias
            // - ClearZBuffer
            // - CullMode
            //
            // Actor::setTextureRenderStates
            // - texturewrappoing
            // - texturefiltering 
            // - effectmode
            //   - See gl 1 renderer - Shaders
            //   - If there's only ever 1 texture for the sprite then copy from there
            //   - If there's multiple textures it's tricky?
            //   - Likely means we need shader preprocessing to build the relevant combinations..

            // RageSpriteVertex
            struct Vertex {
                float p[3] = {0.0f, 0.0f, 0.0f}; // position
                float n[3] = {0.0f, 0.0f, 0.0f}; // normal
                float c[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // colour
                float t[2] = {0.0f, 0.0f}; // texcoord
                float b[2] = {0.0f, 0.0f}; // bboxcoord (managed by drawable, do not modify)
            };

            // Actor::DrawPrimitives -> 1 - 5 DrawTexture calls
            // TODO: For now, vertices still edited in Sprite.cpp
            //       but what should happen is the Sprite parameters
            //       should be passed to a geometry shader.
            // TODO: Potentially, geometry shaders can be quite slow
            //       so would need to compare cpu-side vertex munging
            //       to gpu-side calculations.
            // TODO: Either way, the colour and fade data could probably
            //       be done in shaders, rather than for every sprite
            //       on every draw.

            // TODO: DrawPrimitives
            std::array<Vertex, 4> quadInside;
            bool drawInside = false;
            std::array<Vertex, 4> quadShadow;
            bool drawShadow = false;
            float shadowModelViewMatrix[4][4]; // TODO: Matrix shift in drawable - needs access to matrix stack though
            std::array<Vertex, 4> quadGlow;
            bool drawGlow = false;

            // Fade size as fraction of sprite size
            // left, bottom, top, right
            float fadeSize[4] = {0.0f, 0.0f, 0.0f, 0.0f};
            float cropSize[4] = {0.0f, 0.0f, 0.0f, 0.0f};

            // Call after editing vertices.
            // infrequently if possible to minimise gpu buffer uploads.
            // TODO: Sprite should modify verts through matrices if possible,
            //       rather than editing the vertices directly
            void dirty() { mDirty = true; }

            unsigned int texture0 = 0;

        protected:
            DrawableSprite() {
                // Set bbox coords for each quad. These are set once and never modified,
                // and just mark a 0 -> 1 across whatever vertices are rendered.
                // This is used for rendering fades, which start at the cropped edges of
                // a sprite, and finish some fraction of the way across the sprite.

                // Top left
                quadShadow[0].b[0] = 0.0f;
                quadShadow[0].b[1] = 0.0f;
                quadInside[0].b[0] = 0.0f;
                quadInside[0].b[1] = 0.0f;
                quadGlow[0].b[0] = 0.0f;
                quadGlow[0].b[1] = 0.0f;

                // Bottom left
                quadShadow[1].b[0] = 0.0f;
                quadShadow[1].b[1] = 1.0f;
                quadInside[1].b[0] = 0.0f;
                quadInside[1].b[1] = 1.0f;
                quadGlow[1].b[0] = 0.0f;
                quadGlow[1].b[1] = 1.0f;

                // Bottom right
                quadShadow[2].b[0] = 1.0f;
                quadShadow[2].b[1] = 1.0f;
                quadInside[2].b[0] = 1.0f;
                quadInside[2].b[1] = 1.0f;
                quadGlow[2].b[0] = 1.0f;
                quadGlow[2].b[1] = 1.0f;

                // Top left
                quadShadow[3].b[0] = 1.0f;
                quadShadow[3].b[1] = 0.0f;
                quadInside[3].b[0] = 1.0f;
                quadInside[3].b[1] = 0.0f;
                quadGlow[3].b[0] = 1.0f;
                quadGlow[3].b[1] = 0.0f;
            }
            bool mDirty = true;
    };
}
