#include "global.h"

#include "CalmDrawableMultiTextureOpenGL.h"

#include <GL/glew.h>
#include "GL/gl.h"

#include "calm/opengl/CalmDisplayOpenGL.h"

#include <vector>
#include <limits>

namespace calm
{
    DrawableMultiTextureOpenGL::DrawableMultiTextureOpenGL() {}
    DrawableMultiTextureOpenGL::~DrawableMultiTextureOpenGL() {}

    bool DrawableMultiTextureOpenGL::doValidate(Display* display)
    {
        glGenBuffers(1, &mVBO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        uploadVBO();

        glGenBuffers(1, &mIBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        uploadIBO();

        loadShader(display);

        mDirty = false;
        return true;
    }

    void DrawableMultiTextureOpenGL::doDraw(Display* display)
    {
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);

        if (mDirty)
        {
            uploadVBO();
            uploadIBO();
            mDirty = false;
        }

        DEBUG_GROUP("MultiTexture draw");
        // TODO: Would be nice not to do this - VAOs even ;)
        shader->configureVertexAttributes(ShaderProgram::VertexType::MultiTexture);

        bindShaderAndSetUniforms(shader, modelViewMatrix);

        // TODO: No magic numbers, seriously
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, reinterpret_cast<const void*>(0 * sizeof(GLuint)));
     }

    void DrawableMultiTextureOpenGL::doInvalidate(Display* display)
    {
        shader->invalidate();
        glDeleteBuffers(1, &mVBO);
        glDeleteBuffers(1, &mIBO);
        mVBO = 0;
        mIBO = 0;
    }

    void DrawableMultiTextureOpenGL::uploadVBO() {
        std::vector<DrawableMultiTexture::Vertex> verts;
        verts.insert(std::end(verts), std::begin(quad), std::end(quad));
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(DrawableMultiTexture::Vertex), verts.data(), GL_STATIC_DRAW);
    }

    void DrawableMultiTextureOpenGL::uploadIBO() {
        std::vector<GLuint> indices;

        // TODO: Some neater method of working out the draw indices - We want as few draw calls as possible,
        //       so ideally order shadow, inside, fades, glow, or whatever minimises it if we can.
        // TODO: Or, move as much as possible over to the shaders, so we can avoid all this cpu hackery.
        // TODO: Since the IBO is static and we do a few draw calls into it, it could be static and
        //       only ever uploaded once! It's impossible to be invalid even, so save some time.
        indices.insert(indices.end(), { 2, 1, 0, 3, 2, 0 });
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);        
    }

    void DrawableMultiTextureOpenGL::bindShaderAndSetUniforms(std::shared_ptr<ShaderProgram> shader, float modelView[4][4])
    {
        shader->bind();
        shader->uniformMatrix4fv("modelViewMat", modelView);
        shader->uniformMatrix4fv("projectionMat", projectionMatrix);
        shader->uniformMatrix4fv("textureMat", textureMatrix);

        ASSERT_M(textures.size() <= 4, "Too many textures requested for multitexture drawable - Shader needs updating");
        for( auto i = 0u; i < textures.size(); ++i )
        {
            auto samplerName = "texture" + std::to_string(i);
            auto enabledName = "texture" + std::to_string(i) + "enabled";
            if( textures[i].enabled )
            {
                glActiveTexture(GL_TEXTURE0 + i);
                glBindTexture(GL_TEXTURE_2D, textures[i].handle);
                shader->uniform1i(samplerName, i);
                shader->uniform1i(enabledName, GL_TRUE);
            }
            else
            {
                shader->uniform1i(enabledName, GL_FALSE);
            }
        }
    }

    void DrawableMultiTextureOpenGL::loadShader(Display* display)
    {
        auto params = DisplayOpenGL::ShaderParameters{
          DisplayOpenGL::ShaderBase::MultiTexture,
          {
            DisplayOpenGL::ShaderFeature::TextureUnit0,
            DisplayOpenGL::ShaderFeature::TextureUnit1,
            DisplayOpenGL::ShaderFeature::TextureUnit2,
            DisplayOpenGL::ShaderFeature::TextureUnit3,
          }
        };
        if( textures.size() >= 1 )
        {
            params.textureMode0 = textures[0].textureMode;
        }
        if( textures.size() >= 2 )
        {
            params.textureMode1 = textures[1].textureMode;
        }
        if( textures.size() >= 3 )
        {
            params.textureMode2 = textures[2].textureMode;
        }
        if( textures.size() >= 4 )
        {
            params.textureMode3 = textures[3].textureMode;
        }
        params.effectMode = effectMode;

        shader = dynamic_cast<DisplayOpenGL*>(display)->getShader(params);
    }
}
