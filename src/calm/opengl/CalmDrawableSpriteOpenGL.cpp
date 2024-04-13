
#include "CalmDrawableSpriteOpenGL.h"

#include <GL/glew.h>
#include "GL/gl.h"

namespace calm
{
    DrawableSpriteOpenGL::DrawableSpriteOpenGL() {}
    DrawableSpriteOpenGL::~DrawableSpriteOpenGL() {}

    bool DrawableSpriteOpenGL::doValidate()
    {
        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        shader->configureVertexAttributes(ShaderProgram::VertexType::Sprite);

        mDirty = false;
        return true;
    }

    void DrawableSpriteOpenGL::doDraw()
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        shader->bind();

        if (mDirty)
        {
            // Valid, but the vertex data is outdated, refresh it
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            mDirty = false;
        }

        shader->uniformMatrix4fv("modelViewMat", modelViewMatrix);
        shader->uniformMatrix4fv("projectionMat", projectionMatrix);
        shader->uniformMatrix4fv("textureMat", textureMatrix);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture0);
        shader->uniform1i("texture0", 0);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void DrawableSpriteOpenGL::doInvalidate()
    {
        shader->invalidate();
        glDeleteBuffers(1, &mVBO);
    }
}
