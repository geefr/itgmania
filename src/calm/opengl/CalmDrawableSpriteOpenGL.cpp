
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

        glGenBuffers(1, &mIBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        // GLuint indices[6] = {0, 1, 2, 0, 2, 3};
        GLuint indices[6] = {2, 1, 0, 3, 2, 0};
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), &indices, GL_STATIC_DRAW);

        mDirty = false;
        return true;
    }

    void DrawableSpriteOpenGL::doDraw()
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        // TODO: Would be nice not to do this - VAOs even ;)
        shader->configureVertexAttributes(ShaderProgram::VertexType::Sprite);

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

        if( texture0 != 0 ) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture0);
            shader->uniform1i("texture0", 0);
            shader->uniform1i("texture0Enabled", GL_TRUE);
        }
        else
        {
            shader->uniform1i("texture0Enabled", GL_FALSE);
        }

        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    void DrawableSpriteOpenGL::doInvalidate()
    {
        shader->invalidate();
        glDeleteBuffers(1, &mVBO);
    }
}
