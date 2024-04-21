#pragma once

#include "calm/drawables/CalmDrawableClear.h"

namespace calm {
    class DrawableClearOpenGL final : public DrawableClear {
        public:
            DrawableClearOpenGL();
            ~DrawableClearOpenGL() override;

        protected:
            bool doValidate(Display* display) override;
            void doDraw(Display* display) override;
            void doInvalidate(Display* display) override;
    };
}
