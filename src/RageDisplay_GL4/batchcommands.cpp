
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

	//GLuint BatchDrawCommand::mVAO = 0;
	//GLuint BatchDrawCommand::mVBO = 0;
	//GLuint BatchDrawCommand::mIBO = 0;

	BatchDrawCommand::BatchDrawCommand()
	{
		if (mVBO == 0) glGenBuffers(1, &mVBO);
		if (mIBO == 0) glGenBuffers(1, &mIBO);
		if (mVAO == 0) glGenVertexArrays(1, &mVAO);
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

	SpriteVertexDrawElementsCommand::SpriteVertexDrawElementsCommand(GLenum drawMode)
		: BatchDrawCommand()
		, mDrawMode(drawMode)
	{
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		ShaderProgram::configureVertexAttributesForSpriteRender();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
	}

	SpriteVertexDrawElementsCommand::~SpriteVertexDrawElementsCommand()
	{
	}

	void SpriteVertexDrawElementsCommand::reset()
	{
		mVertices.clear();
		mIndices.clear();
	}

	void SpriteVertexDrawElementsCommand::addDraw(
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
	}

	bool SpriteVertexDrawElementsCommand::canMergeCommand(BatchCommand* cmd)
	{
		if (auto x = dynamic_cast<SpriteVertexDrawElementsCommand*>(cmd))
		{
			return x->mDrawMode == mDrawMode;
		}
		return false;
	}

	void SpriteVertexDrawElementsCommand::mergeCommand(BatchCommand* cmd)
	{
		auto x = dynamic_cast<SpriteVertexDrawElementsCommand*>(cmd);
		addDraw(x->vertices(), x->indices());
	}

	void SpriteVertexDrawElementsCommand::doDispatch()
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
	}

	SpriteVertexDrawArraysCommand::SpriteVertexDrawArraysCommand(GLenum drawMode)
		: BatchDrawCommand()
		, mDrawMode(drawMode)
	{
		glBindVertexArray(mVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		ShaderProgram::configureVertexAttributesForSpriteRender();
	}

	SpriteVertexDrawArraysCommand::~SpriteVertexDrawArraysCommand() {}

	void SpriteVertexDrawArraysCommand::reset()
	{
		vertices.clear();
	}

	bool SpriteVertexDrawArraysCommand::canMergeCommand(BatchCommand* cmd)
	{
		if (auto x = dynamic_cast<SpriteVertexDrawArraysCommand*>(cmd))
		{
			return x->mDrawMode == mDrawMode;
		}
		return false;
	}

	void SpriteVertexDrawArraysCommand::mergeCommand(BatchCommand* cmd)
	{
		auto x = dynamic_cast<SpriteVertexDrawArraysCommand*>(cmd);
		vertices.insert(vertices.end(),
			std::make_move_iterator(x->vertices.begin()),
			std::make_move_iterator(x->vertices.end())
		);
		x->vertices = {};
	}

	void SpriteVertexDrawArraysCommand::doDispatch()
	{
		if (vertices.empty()) return;

		glBindVertexArray(mVAO);

		glBindBuffer(GL_ARRAY_BUFFER, mVBO);
		glBufferData(GL_ARRAY_BUFFER,
			vertices.size() * sizeof(SpriteVertex),
			vertices.data(),
			GL_STREAM_DRAW
		);

		glDrawArrays(mDrawMode, 0, vertices.size());
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
