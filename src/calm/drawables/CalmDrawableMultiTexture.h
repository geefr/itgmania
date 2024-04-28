#pragma once

#include "calm/drawables/CalmDrawable.h"

#include <array>

namespace calm {
    /**
     * Drawable for ActorMultiTexture
     * - Supports 4 textures
     * - Supports effect modes
     * - Doesn't support cropping or fading, as a sprite would
     */
    class DrawableMultiTexture : public Drawable {
        public:
            virtual ~DrawableMultiTexture() {}

            // RageSpriteVertex
            struct Vertex {
                float p[3] = {0.0f, 0.0f, 0.0f}; // position
                float n[3] = {0.0f, 0.0f, 0.0f}; // normal
                float c[4] = {0.0f, 0.0f, 0.0f, 0.0f}; // colour
                float t[2] = {0.0f, 0.0f}; // texcoord
            };

            // Multitexture renders one quad
            std::array<Vertex, 4> quad;

            // And up to 4 textures
            struct TextureParams
            {
                uintptr_t handle = 0;
                bool enabled = false;
                TextureMode textureMode = TextureMode::TextureMode_Modulate;
            };
            std::vector<TextureParams> textures;

            // And an effect mode
            // Note: Effect modes use a specific number of textures,
            // (TODO: And may interact strangely with texture modes)
            EffectMode effectMode = EffectMode::EffectMode_Normal;

            // Call after editing vertices.
            // infrequently if possible to minimise gpu buffer uploads.
            // TODO: Should modify verts through matrices if possible,
            //       rather than editing the vertices directly
            void dirty() { mDirty = true; }

            unsigned int texture0 = 0;

        protected:
            DrawableMultiTexture() {}
            bool mDirty = true;
    };
}
