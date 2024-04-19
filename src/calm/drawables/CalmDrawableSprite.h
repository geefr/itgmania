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
            // std::array<Vertex, 4> quadFadeLeft;
            // bool drawFadeLeft = false;
            // std::array<Vertex, 4> quadFadeRight;
            // bool drawFadeRight = false;
            // std::array<Vertex, 4> quadFadeTop;
            // bool drawFadeTop = false;
            // std::array<Vertex, 4> quadFadeBottom;
            // bool drawFadeBottom = false;

            std::array<Vertex, 4> quadShadow;
            bool drawShadow = false;
            float shadowLengthX = 5.0;
            float shadowLengthY = 5.0;
            std::array<Vertex, 4> quadGlow;
            bool drawGlow = false;

            // Call after editing vertices.
            // infrequently if possible to minimise gpu buffer uploads.
            // TODO: Sprite should modify verts through matrices if possible,
            //       rather than editing the vertices directly
            void dirty() { mDirty = true; }

            unsigned int texture0 = 0;

        protected:
            DrawableSprite() {}
            bool mDirty = true;
    };
}
