
#include "global.h"

#include "CalmDrawableSpriteOpenGL.h"

#include <GL/glew.h>
#include "GL/gl.h"

#include <vector>
#include <limits>

namespace calm
{
    DrawableSpriteOpenGL::DrawableSpriteOpenGL() {}
    DrawableSpriteOpenGL::~DrawableSpriteOpenGL() {}

    bool DrawableSpriteOpenGL::doValidate(Display* display)
    {
        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        uploadVBO();

        glGenBuffers(1, &mIBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        uploadIBO();

        mDirty = false;
        return true;
    }

    void DrawableSpriteOpenGL::doDraw(Display* display)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);

        if (mDirty)
        {
            uploadVBO();
            uploadIBO();
            mDirty = false;
        }

        auto drawModulate = drawInside || drawShadow;
        drawModulate &= mDrawModulateN != 0;
        auto drawGlow = mDrawGlowN != 0;

        if( drawModulate )
        {
            DEBUG_GROUP("Sprite draw modulate");
            // TODO: Would be nice not to do this - VAOs even ;)
            shaderModulate->configureVertexAttributes(ShaderProgram::VertexType::Sprite);

            if( drawShadow) {
                // TODO: Inefficient - Should be able to render shadow and body together, and not have multiple uniforms/draws..
                bindShaderAndSetUniforms(shaderModulate, shadowModelViewMatrix);

                // TODO: No magic numbers, seriously
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, reinterpret_cast<const void*>(0 * sizeof(GLuint)));
            }

            if( drawInside ) { 
                bindShaderAndSetUniforms(shaderModulate, modelViewMatrix);

                // TODO: No magic numbers, seriously
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, reinterpret_cast<const void*>(6 * sizeof(GLuint)));
            }
            // glDrawElements(GL_TRIANGLES, mDrawModulateN, GL_UNSIGNED_INT, reinterpret_cast<const void*>(mDrawModulateStart * sizeof(GLuint)));
        }

        if( drawGlow )
        {
            DEBUG_GROUP("Sprite draw glow");
            // TODO: Would be nice not to do this - VAOs even ;)
            shaderGlow->configureVertexAttributes(ShaderProgram::VertexType::Sprite);
            bindShaderAndSetUniforms(shaderGlow, modelViewMatrix);

            // TODO: No magic numbers, seriously
            // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, reinterpret_cast<const void*>(12 * sizeof(GLuint)));
            glDrawElements(GL_TRIANGLES, mDrawGlowN, GL_UNSIGNED_INT, reinterpret_cast<const void*>(mDrawGlowStart * sizeof(GLuint)));
        }
     }

    void DrawableSpriteOpenGL::doInvalidate(Display* display)
    {
        shaderModulate->invalidate();
        shaderGlow->invalidate();
        mDrawModulateStart = 0;
        mDrawModulateN = 0;
        mDrawGlowStart = 0;
        mDrawGlowN = 0;
        glDeleteBuffers(1, &mVBO);
        glDeleteBuffers(1, &mIBO);
        mVBO = 0;
        mIBO = 0;
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

        mDrawModulateStart = std::numeric_limits<GLuint>::max();
        mDrawModulateN = 0;
        mDrawGlowStart = 0;
        mDrawGlowN = 0;

        // TODO: Some neater method of working out the draw indices - We want as few draw calls as possible,
        //       so ideally order shadow, inside, fades, glow, or whatever minimises it if we can.
        // TODO: Or, move as much as possible over to the shaders, so we can avoid all this cpu hackery.
        // TODO: Since the IBO is static and we do a few draw calls into it, it could be static and
        //       only ever uploaded once! It's impossible to be invalid even, so save some time.
        indices.insert(indices.end(), { 2, 1, 0, 3, 2, 0 });
        if( drawShadow ) {
            mDrawModulateStart = std::min(mDrawModulateStart, 0u);
            mDrawModulateN += 6;
        }
        indices.insert(indices.end(), { 6, 5, 4, 7, 6, 4 });
        if( drawInside ) {
            mDrawModulateStart = std::min(mDrawModulateStart, 6u);
            mDrawModulateN += 6;
        }
        indices.insert(indices.end(), { 10, 9, 8, 11, 10, 8 });
        if( drawGlow ) {
            // Position of glow verts in vbo - fixed
            mDrawGlowStart = 12;
            mDrawGlowN = 6;
        }

        // if( indices.empty() ) {indices = {2, 1, 0, 3, 2, 0};}; // Have something, to initialise the buffer 
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);        
    }

    void DrawableSpriteOpenGL::bindShaderAndSetUniforms(std::shared_ptr<ShaderProgram> shader, float modelView[4][4])
    {
        shader->bind();
        shader->uniformMatrix4fv("modelViewMat", modelView);
        shader->uniformMatrix4fv("projectionMat", projectionMatrix);
        shader->uniformMatrix4fv("textureMat", textureMatrix);
        if( texture0 != 0 ) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture0);
            shader->uniform1i("texture0", 0);
            shader->uniform1i("texture0enabled", GL_TRUE);
        }
        else
        {
            shader->uniform1i("texture0enabled", GL_FALSE);
        }
        shader->uniform4f("fadeSize", fadeSize);
        shader->uniform4f("cropSize", cropSize);
    }
}
