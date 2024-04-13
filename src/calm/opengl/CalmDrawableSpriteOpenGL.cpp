
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
        shader->configureVertexAttributes(ShaderProgram::VertexType::Sprite);

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

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

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    void DrawableSpriteOpenGL::doInvalidate()
    {
        shader->invalidate();
        glDeleteBuffers(1, &mVBO);
    }
}
