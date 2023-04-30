
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

	SpriteVertexDrawElementsCommand::SpriteVertexDrawElementsCommand(GLenum drawMode)
		: mDrawMode(drawMode)
	{}

	SpriteVertexDrawElementsCommand::~SpriteVertexDrawElementsCommand() {}

	void SpriteVertexDrawElementsCommand::reset()
	{
		vertices = {};
		indices = {};
		indexBufferOffset = 0;
		drawNumIndices = 0;
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

		auto baseVertex = vertices.size();
		for (auto& i : x->indices) i += baseVertex;
		drawNumIndices += x->indices.size();

		vertices.insert(vertices.end(),
			std::make_move_iterator(x->vertices.begin()),
			std::make_move_iterator(x->vertices.end())
		);

		indices.insert(indices.end(),
			std::make_move_iterator(x->indices.begin()),
			std::make_move_iterator(x->indices.end())
		);
	}

	void SpriteVertexDrawElementsCommand::setRenderInstance(uint32_t renderInstance)
	{
		for( auto& v : vertices ) v.renderInstance = renderInstance;
	}

	void SpriteVertexDrawElementsCommand::appendDataToBuffer(std::vector<SpriteVertex>& bufferVertices, std::vector<GLuint>& bufferIndices)
	{
		auto baseVertex = bufferVertices.size();
		for (auto& i : indices) i += baseVertex;

		drawNumIndices = indices.size();
		indexBufferOffset = bufferIndices.size();

		bufferVertices.insert(bufferVertices.end(),
			std::make_move_iterator(vertices.begin()),
			std::make_move_iterator(vertices.end())
		);

		bufferIndices.insert(bufferIndices.end(),
			std::make_move_iterator(indices.begin()),
			std::make_move_iterator(indices.end())
		);
	}

	void SpriteVertexDrawElementsCommand::doDispatch()
	{
		if( drawNumIndices == 0 ) return;
		glDrawElements(
			mDrawMode,
			drawNumIndices,
			GL_UNSIGNED_INT,
			reinterpret_cast<const void*>(indexBufferOffset * sizeof(GLuint))
		);
	}

	SpriteVertexDrawArraysCommand::SpriteVertexDrawArraysCommand(GLenum drawMode)
		: mDrawMode(drawMode)
	{}

	SpriteVertexDrawArraysCommand::~SpriteVertexDrawArraysCommand() {}

	void SpriteVertexDrawArraysCommand::reset()
	{
		vertices = {};
		drawNumVertices = 0;
		drawFirstVertex = 0;
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
	}

	void SpriteVertexDrawArraysCommand::setRenderInstance(uint32_t renderInstance)
	{
		for (auto& v : vertices) v.renderInstance = renderInstance;
	}

	void SpriteVertexDrawArraysCommand::appendDataToBuffer(std::vector<SpriteVertex>& bufferVertices, std::vector<GLuint>& bufferIndices)
	{
		drawNumVertices = vertices.size();
		drawFirstVertex = bufferVertices.size();

		bufferVertices.insert(bufferVertices.end(),
			std::make_move_iterator(vertices.begin()),
			std::make_move_iterator(vertices.end())
		);
	}

	void SpriteVertexDrawArraysCommand::doDispatch()
	{
		if( drawNumVertices == 0 ) return;
		glDrawArrays(mDrawMode, drawFirstVertex, drawNumVertices);
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

	void ClearCommand::setRenderInstance(uint32_t renderInstance)
	{
		// NOP
	}

	CompiledGeometryDrawCommand::CompiledGeometryDrawCommand(const CompiledGeometry* geom, int meshIndex)
	  : mGeom(geom)
	  , mMeshIndex(meshIndex)
	{}

	CompiledGeometryDrawCommand::~CompiledGeometryDrawCommand() {}

	void CompiledGeometryDrawCommand::reset()
	{
		mGeom = nullptr;
		mMeshIndex = 0;
	}

	void CompiledGeometryDrawCommand::doDispatch()
	{
		if( !mGeom ) return;
		mGeom->Draw(mMeshIndex);
	}
}
