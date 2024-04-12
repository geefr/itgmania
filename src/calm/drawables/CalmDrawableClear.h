#pragma once

#include "calm/drawables/CalmDrawable.h"

namespace calm {
    /**
     * Clear colour or depth buffers
     */
    class DrawableClear : public Drawable {
        public:
            virtual ~DrawableClear() {}

            bool clearColour = true;
            float clearColourR = 0.0f;
            float clearColourG = 0.0f;
            float clearColourB = 0.0f;
            float clearColourA = 0.0f;

            bool clearDepth = true;

        protected:
            DrawableClear() {}
    };
}
