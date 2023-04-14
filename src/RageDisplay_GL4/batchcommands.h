#pragma once

#include <GL/glew.h>

#include "gl4types.h"

#include <vector>

namespace RageDisplay_GL4
{

class Batch
{
    public:
        virtual ~Batch();

        virtual void flush() = 0;
        virtual void clear() = 0;
    protected:
        Batch();

    GLuint mVAO = 0;
    GLuint mVBO = 0;
    GLuint mIBO = 0;
};

class SpriteVertexBatch : public Batch
{
    public:
        SpriteVertexBatch() = delete;
        SpriteVertexBatch(GLenum drawMode);
        virtual ~SpriteVertexBatch();
    
        void flush() override;
        void clear() override;

        void addDraw(
            std::vector<SpriteVertex>&& vertices,
            std::vector<GLuint>&& indices);

    private:
        std::vector<SpriteVertex> mVertices;
        std::vector<GLuint> mIndices;
        GLenum mDrawMode = GL_TRIANGLES;
        unsigned int mNumBatched = 0;
};

}
