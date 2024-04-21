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

            std::shared_ptr<ShaderProgram> shaderModulate0;
            std::shared_ptr<ShaderProgram> shaderGlow0;

        protected:
            bool doValidate(Display* display) override;
            void doDraw(Display* display) override;
            void doInvalidate(Display* display) override;

        private:
            void uploadVBO();
            void uploadIBO();
            void bindShaderAndSetUniforms(std::shared_ptr<ShaderProgram> shader, float modelView[4][4]);
            GLuint mVBO = 0;
            GLuint mIBO = 0;

            GLuint mDrawModulateStart = 0;
            GLuint mDrawModulateN = 0;
            GLuint mDrawGlowStart = 0;
            GLuint mDrawGlowN = 0;
    };
}
