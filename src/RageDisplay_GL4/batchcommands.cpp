
#include "batchcommands.h"

#include "shaderprogram.h"

#include <RageLog.h>

namespace RageDisplay_GL4
{
	void BatchCommand::dispatch(const State& previousState)
	{
		state.updateGPUState(previousState);
		doDispatch();
	}

	void BatchCommand::dispatch()
	{
		state.updateGPUState();
		doDispatch();
	}

	BatchDrawCommand::BatchDrawCommand()
	{
		glGenBuffers(1, &mVBO);
		glGenBuffers(1, &mIBO);
		glGenVertexArrays(1, &mVAO);
	}

	BatchDrawCommand::~BatchDrawCommand()
	{
		if (mIBO)
			glDeleteBuffers(1, &mIBO);
		if (mVBO)
			glDeleteBuffers(1, &mVBO);
		if (mVAO)
			glDeleteVertexArrays(1, &mVAO);
	}

	SpriteVertexDrawCommand::SpriteVertexDrawCommand(GLenum drawMode)
		: BatchDrawCommand()
		, mDrawMode(drawMode)
	{
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		ShaderProgram::configureVertexAttributesForSpriteRender();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
	}

	SpriteVertexDrawCommand::~SpriteVertexDrawCommand()
	{
	}

	void SpriteVertexDrawCommand::doDispatch()
	{
		if (mIndices.empty()) return;

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
	}

	void SpriteVertexDrawCommand::reset()
	{
		mVertices.clear();
		mIndices.clear();
		mNumBatched = 0;
	}

	void SpriteVertexDrawCommand::addDraw(
		std::vector<SpriteVertex>&& vertices,
		std::vector<GLuint>&& indices)
	{
		auto baseVertex = mVertices.size();
		for (auto& i : indices) i += baseVertex;

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

	bool SpriteVertexDrawCommand::canMergeCommand(BatchCommand* cmd)
	{
		if (auto x = dynamic_cast<SpriteVertexDrawCommand*>(cmd))
		{
			return x->mDrawMode == mDrawMode;
		}
	}

	void SpriteVertexDrawCommand::mergeCommand(BatchCommand* cmd)
	{
		auto x = dynamic_cast<SpriteVertexDrawCommand*>(cmd);
		addDraw(x->vertices(), x->indices());
	}

	ClearCommand::ClearCommand() {}

	ClearCommand::~ClearCommand() {}

	void ClearCommand::doDispatch()
	{
		glClear(mask);
	}

	void ClearCommand::reset()
	{
		mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;
	}

	bool ClearCommand::canMergeCommand(BatchCommand* cmd)
	{
		return dynamic_cast<ClearCommand*>(cmd) != nullptr;
	}

	void ClearCommand::mergeCommand(BatchCommand* cmd)
	{
		auto x = dynamic_cast<ClearCommand*>(cmd);
		mask |= x->mask;
	}

}
