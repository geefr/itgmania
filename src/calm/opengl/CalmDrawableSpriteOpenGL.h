#pragma once

#include "calm/drawables/CalmDrawableSprite.h"
#include "calm/opengl/CalmShaderTypesOpenGL.h"
#include "calm/opengl/CalmShaderProgramOpenGL.h"

#include <memory>

namespace calm {
    class DrawableSpriteOpenGL final : public DrawableSprite {
        public:
            DrawableSpriteOpenGL();
            ~DrawableSpriteOpenGL() override;

            std::shared_ptr<ShaderProgram> shader;

        protected:
            bool doValidate() override;
            void doDraw() override;
            void doInvalidate() override;

            GLuint mVBO = 0;
            GLuint mIBO = 0;
    };
}
