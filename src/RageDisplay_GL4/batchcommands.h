#pragma once

#include <GL/glew.h>

#include "gl4types.h"
#include "gl4state.h"

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
        SpriteVertexBatch(GLenum drawMode, State state);
        virtual ~SpriteVertexBatch();
    
        void flush() override;
				void flush(const State& previousState);
        void clear() override;

        void addDraw(
            std::vector<SpriteVertex>&& vertices,
            std::vector<GLuint>&& indices);

				State mState;
				std::vector<SpriteVertex> vertices() { return mVertices; }
				std::vector<GLuint> indices() { return mIndices; }

    private:
				void doFlush();
        std::vector<SpriteVertex> mVertices;
        std::vector<GLuint> mIndices;
        GLenum mDrawMode = GL_TRIANGLES;
        unsigned int mNumBatched = 0;
};

}
