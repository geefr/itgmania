#pragma once

#include "calm/drawables/CalmDrawable.h"

namespace calm {
    /**
     * Drawable for sprite / textured quad rendering
     */
    class DrawableSprite : public Drawable {
        public:
            virtual ~DrawableSprite() {}

            // RageSpriteVertex
            struct Vertex {
                float p[3]; // position
                float n[3]; // normal
                float c[4]; // colour
                float t[2]; // texcoord
            };
            Vertex vertices[4];

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
