
#include "batchcommands.h"

#include "shaderprogram.h"

#include <RageLog.h>

namespace RageDisplay_GL4
{
    Batch::Batch()
    {
        glGenBuffers(1, &mVBO);
        glGenBuffers(1, &mIBO);
        glGenVertexArrays(1, &mVAO);
    }

    Batch::~Batch()
    {
        if (mIBO)
            glDeleteBuffers(1, &mIBO);
        if (mVBO)
            glDeleteBuffers(1, &mVBO);
        if (mVAO)
            glDeleteVertexArrays(1, &mVAO);
    }

    SpriteVertexBatch::SpriteVertexBatch(GLenum drawMode, State state)
      : Batch()
      , mDrawMode(drawMode)
			, mState(state)
    {
        glBindVertexArray(mVAO);
        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        ShaderProgram::configureVertexAttributesForSpriteRender();
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
    }

    SpriteVertexBatch::~SpriteVertexBatch()
    {
    }

	void SpriteVertexBatch::flush(const State& previousState)
	{
		mState.updateGPUState(previousState);
		doFlush();
	}

	void SpriteVertexBatch::flush()
	{
		mState.updateGPUState();
		doFlush();
	}

		void SpriteVertexBatch::doFlush()
    {
        if( mIndices.empty() ) return;

        glBindVertexArray(mVAO);

        glBindBuffer(GL_ARRAY_BUFFER, mVBO);
        glBufferData(GL_ARRAY_BUFFER,
            mVertices.size() * sizeof(SpriteVertex),
            mVertices.data(),
            GL_STREAM_DRAW
        );
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
            mIndices.size() * sizeof(GLuint),
            mIndices.data(),
            GL_STREAM_DRAW
        );

        glDrawElements(
            mDrawMode,
            mIndices.size(),
            GL_UNSIGNED_INT,
            nullptr
        );

        // LOG->Info("Num batched: %d", mNumBatched);
        clear();
    }

    void SpriteVertexBatch::clear()
    {
        mVertices.clear();
        mIndices.clear();
        mNumBatched = 0;
    }

    void SpriteVertexBatch::addDraw(
        std::vector<SpriteVertex>&& vertices,
        std::vector<GLuint>&& indices)
    {
        auto baseVertex = mVertices.size();
        for( auto& i : indices ) i += baseVertex;

        mVertices.insert(mVertices.end(),
            std::make_move_iterator(vertices.begin()),
            std::make_move_iterator(vertices.end())
        );

        mIndices.insert(mIndices.end(),
            std::make_move_iterator(indices.begin()),
            std::make_move_iterator(indices.end())
        );

        mNumBatched++;
    }
}
