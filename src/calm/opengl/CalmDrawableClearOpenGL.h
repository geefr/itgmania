#pragma once

#include "calm/drawables/CalmDrawableClear.h"

namespace calm {
    class DrawableClearOpenGL final : public DrawableClear {
        public:
            DrawableClearOpenGL();
            ~DrawableClearOpenGL() override;

        protected:
            bool doValidate() override;
            void doDraw() override;
            void doInvalidate() override;
    };
}
