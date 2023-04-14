#pragma once

#include <RageDisplay.h>

#include <GL/glew.h>

#include "gl4types.h"
#include "shaderprogram.h"
#include "batchcommands.h"

#include <memory>

namespace RageDisplay_GL4
{

  /**
   * Rendering adapter class that batches individual draw calls from RageDisplay_GL4,
   * and dispatches to OpenGL in a somewhat more efficient manner.
   *
   * Also performs state tracking for common rendering settings such as lighting
   * and texture parameters.
   *
   * Workflow for this class is:
   * * Create the renderer
   * * Configure state as needed
   * * Call drawXXX as needed
   * *
   */
  class BatchedRenderer
  {
  public:
    BatchedRenderer();
    ~BatchedRenderer();

    void init();

    /// Immediately flush all batches and pending commands to OpenGL
    void flushBatches();

    /// flush batches and glClear
    void clear();
    void clearDepthBuffer();

          /// Primitive draw calls
          void drawQuads(const RageSpriteVertex v[], int numVerts);
  /*
          void drawQuadStrip();
          void drawTriangleFan();
          void drawTriangleStrip();
          void drawTriangles();
          void drawLines()
          void drawSymmetricQuadStrip();

          /// CompiledGeometry draws are batched across meshes
          /// if possible, but cannot be batched with primitive
          /// draws, or across multiple different models
          void drawCompiledGeometry();
  */
    /// Global state changes
    /// Will flush batches when modified
    bool depthWriteEnabled() const { return mDepthWriteEnabled; }
    GLenum depthFunc() const { return mDepthFunc; }

    void depthWrite(bool enable);
    void depthFunc(GLenum func);
    void depthRange(float zNear, float zFar);
    void blendMode(GLenum eq, GLenum sRGB, GLenum dRGB, GLenum sA, GLenum dA);
    void cull(bool enable, GLenum face);
    void alphaTest(bool enable);
    void lineWidth(float w);
    void pointSize(float s);

    /// Make the renderer aware of textures
    /// The renderer manages per-texture state,
    /// but texture creation & deletion is
    /// managed by the caller (To allow async uploads and similar)
    void addTexture(GLuint tex, bool hasMipMaps);
    bool textureHasMipMaps(GLuint tex) const;
    void removeTexture(GLuint tex);
    void invalidateTexture(GLuint tex);

    /// Per-texture-unit state
    /// Any change to bound textures will trigger
    /// a batch flush
    // TODO: This could be reduced, if more complex
    //       texture binding, or bindless textures
    //       were used - The shader can handle at
    //       least 16 textures at a time, and
    //       stepmania needs at most 4 at a time.
    void bindTexture(GLuint unit, GLuint tex);
    GLuint boundTexture(GLuint unit) const;
    void textureWrap(GLuint unit, GLenum mode);
    void textureFilter(GLuint unit, GLenum minFilter, GLenum magFilter);

    /// Shader program
    /// Any change to active program,
    /// or its uniforms will trigger a batch flush
    //      void shader(ShaderProgram& s);

  private:
    struct TextureState
    {
        bool hasMipMaps = false;

        // Strictly speaking these are per-texture-sampler
        // settings, but currently we use glTextureParameter
        // and each texture has its own built-in sampler.
        // This makes state tracking a little awkward,
        // but is inline with the legacy renderer / SM's expectations.
        GLenum wrapMode = GL_REPEAT;
        GLenum minFilter = GL_NEAREST_MIPMAP_LINEAR;
        GLenum magFilter = GL_LINEAR;
    };

    void globalStateChanged();
    void lineOrPointStateChanged();
    //	void shaderStateChanged();
    void textureStateChanged();

    // Global state
    bool mDepthWriteEnabled = true;
    GLenum mDepthFunc = GL_LESS;
    float mDepthNear = 0.0f;
    float mDepthFar = 1.0f;
    GLenum mBlendEq = GL_FUNC_ADD;
    GLenum mBlendSourceRGB = GL_ONE;
    GLenum mBlendDestRGB = GL_ZERO;
    GLenum mBlendSourceAlpha = GL_ONE;
    GLenum mBlendDestAlpha = GL_ZERO;
    bool mCullEnabled = false;
    GLenum mCullFace = GL_BACK;
    float mLineWidth = 1.0f;
    float mPointSize = 1.0f;

    // TODO
    // Alpha test is actually shader state, not global
    // But if it changes we need to flush all the same
    // bool mAlphaTestEnabled = false;

    std::map<GLuint, TextureState> mTextureState;
    std::map<GLuint, GLuint> mBoundTextures;

    // Batch buffers
    std::unique_ptr<SpriteVertexBatch> mQuadsBatch;
  };

}

/*
 * Copyright (c) 2023 Gareth Francis
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
