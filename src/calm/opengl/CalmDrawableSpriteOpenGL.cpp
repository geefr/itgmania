
#include "CalmDrawableSpriteOpenGL.h"

#include <GL/glew.h>
#include "GL/gl.h"

#include <vector>

namespace calm
{
    DrawableSpriteOpenGL::DrawableSpriteOpenGL() {}
    DrawableSpriteOpenGL::~DrawableSpriteOpenGL() {}

    bool DrawableSpriteOpenGL::doValidate()
    {
        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        uploadVBO();
        shader->configureVertexAttributes(ShaderProgram::VertexType::Sprite);

        glGenBuffers(1, &mIBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        uploadIBO();

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
            uploadVBO();
            uploadIBO();
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

        // TODO: Now that I think about it, could have a static IBO just carefully organised
        //       then start rendering from a given index whether fade is enabled / not
        //       or do 1 draw / multi draws if needed to hop over the glow pass or similar?
        glDrawElements(GL_TRIANGLES, mIBOCount, GL_UNSIGNED_INT, 0);
    }

    void DrawableSpriteOpenGL::doInvalidate()
    {
        shader->invalidate();
        glDeleteBuffers(1, &mVBO);
    }

    void DrawableSpriteOpenGL::uploadVBO() {
        std::vector<DrawableSprite::Vertex> verts;
        verts.insert(std::end(verts), std::begin(quadShadow), std::end(quadShadow));
        verts.insert(std::end(verts), std::begin(quadInside), std::end(quadInside));
        verts.insert(std::end(verts), std::begin(quadGlow), std::end(quadGlow));
        
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(DrawableSprite::Vertex), verts.data(), GL_STATIC_DRAW);
    }

    void DrawableSpriteOpenGL::uploadIBO() {
        std::vector<GLuint> indices;

        if( drawShadow ) {
            indices.insert(indices.end(), { 2, 1, 0, 3, 2, 0 });
        }
        if( drawInside ) {
            indices.insert(indices.end(), { 6, 5, 4, 7, 6, 4 });
        }
        // TODO: Probably needs to be a separate ibo / similar - different shader uniforms?
        if( drawGlow ) {
            indices.insert(indices.end(), { 10, 9, 8, 11, 10, 8 });
        }

        if( indices.empty() ) {indices = {2, 1, 0, 3, 2, 0};}; // Have something, to initialise the buffer 
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        mIBOCount = indices.size();
    }
}
