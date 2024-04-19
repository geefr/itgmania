
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
        // shader->configureVertexAttributes(ShaderProgram::VertexType::Sprite);

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

        if (mDirty)
        {
            uploadVBO();
            uploadIBO();
        }

        auto drawModulate = drawInside || drawShadow;
        drawModulate &= mDrawModulateN != 0;
        auto drawGlow = mDrawGlowN != 0;

        if( drawModulate )
        {
            // TODO: Would be nice not to do this - VAOs even ;)
            shaderModulate0->configureVertexAttributes(ShaderProgram::VertexType::Sprite);
            bindShaderAndSetUniforms(shaderModulate0);

            // TODO: No magic numbers, seriously
            glDrawElements(GL_TRIANGLES, mDrawModulateN, GL_UNSIGNED_INT, reinterpret_cast<const void*>(mDrawModulateStart));
        }

        if( drawGlow )
        {
            // TODO: Would be nice not to do this - VAOs even ;)
            shaderGlow0->configureVertexAttributes(ShaderProgram::VertexType::Sprite);
            bindShaderAndSetUniforms(shaderGlow0);

            // TODO: No magic numbers, seriously
            glDrawElements(GL_TRIANGLES, mDrawGlowN, GL_UNSIGNED_INT, reinterpret_cast<const void*>(mDrawGlowStart));
        }
    }

    void DrawableSpriteOpenGL::doInvalidate()
    {
        shaderModulate0->invalidate();
        shaderGlow0->invalidate();
        mDrawModulateStart = 0;
        mDrawModulateN = 0;
        mDrawGlowStart = 0;
        mDrawGlowN = 0;
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

        mDrawModulateStart = 0;
        mDrawModulateN = 0;
        mDrawGlowStart = 0;
        mDrawGlowN = 0;

        if( drawShadow ) {
            mDrawModulateStart = 0;
            indices.insert(indices.end(), { 2, 1, 0, 3, 2, 0 });
            mDrawModulateN = indices.size();
        }
        if( drawInside ) {
            indices.insert(indices.end(), { 6, 5, 4, 7, 6, 4 });
            mDrawModulateN = indices.size();
        }
        // TODO: Probably needs to be a separate ibo / similar - different shader uniforms?
        if( drawGlow ) {
            mDrawGlowStart = indices.size();
            indices.insert(indices.end(), { 10, 9, 8, 11, 10, 8 });
            mDrawGlowN = indices.size();
        }

        if( indices.empty() ) {indices = {2, 1, 0, 3, 2, 0};}; // Have something, to initialise the buffer 
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);
        mIBOCount = indices.size();
    }

    void DrawableSpriteOpenGL::bindShaderAndSetUniforms(std::shared_ptr<ShaderProgram> shader)
    {
        shader->bind();
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
    }
}
