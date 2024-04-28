#pragma once

#include "calm/drawables/CalmDrawableMultiTexture.h"
#include "calm/opengl/CalmShaderTypesOpenGL.h"
#include "calm/opengl/CalmShaderProgramOpenGL.h"

#include <memory>

namespace calm {
    class DrawableMultiTextureOpenGL final : public DrawableMultiTexture {
        public:
            DrawableMultiTextureOpenGL();
            ~DrawableMultiTextureOpenGL() override;

            // MultiTexture determines a shader based on
            // the configured texture/effect modes.
            // May be loaded explicitly, or will be
            // updated when drawn.
            void loadShader(Display* display);

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
            std::shared_ptr<ShaderProgram> shader;
    };
}
