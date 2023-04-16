#pragma once

#include <GL/glew.h>

#include "gl4types.h"
#include "gl4state.h"

#include <vector>

namespace RageDisplay_GL4
{
	class BatchCommand
	{
	public:
		enum class VertexType
		{
	    NoVertex,
			Sprite,
			Compiled
		};

		virtual VertexType vertexType() const = 0;

		virtual ~BatchCommand() {};

		virtual void dispatch();
		virtual void dispatch(const State& previousState);
		virtual void reset() = 0;

		// Yes, non-const
		// But all commands _must_ be re-usable after merging, and calling reset()
		virtual bool canMergeCommand(BatchCommand* cmd) { return false; }
		virtual void mergeCommand(BatchCommand* cmd) {}

		// Copy (preferably move) the command's data into shared containers for buffer upload
		// * The command is responsible for adjusting draw indices and storing offsets as required
		// * The position of appended data will not be modified before rendering
		// i.e. add vertices.size() to each index, then append. Store indices.size() for index buffer offset if required.
		virtual void appendDataToBuffer(std::vector<SpriteVertex>& bufferVertices, std::vector<GLuint>& bufferIndices) {};

		State state;
	protected:
		BatchCommand() {};
		virtual void doDispatch() = 0;
	};

	class SpriteVertexDrawElementsCommand : public BatchCommand
	{
	public:
		VertexType vertexType() const override { return VertexType::Sprite; }

		SpriteVertexDrawElementsCommand() = delete;
		SpriteVertexDrawElementsCommand(GLenum drawMode);
		~SpriteVertexDrawElementsCommand() override;

		void reset() override;

		bool canMergeCommand(BatchCommand* cmd) override;
		void mergeCommand(BatchCommand* cmd) override;

		void appendDataToBuffer(std::vector<SpriteVertex>& bufferVertices, std::vector<GLuint>& bufferIndices) override;

		std::vector<SpriteVertex> vertices;
		std::vector<GLuint> indices;

	protected:
		void doDispatch() override;
	
		GLenum mDrawMode = GL_TRIANGLES;

		GLuint drawNumIndices = 0;
		GLuint indexBufferOffset = 0;
	};

	class SpriteVertexDrawArraysCommand : public BatchCommand
	{
	public:
		VertexType vertexType() const override { return VertexType::Sprite; }

		SpriteVertexDrawArraysCommand() = delete;
		SpriteVertexDrawArraysCommand(GLenum drawMode);
		~SpriteVertexDrawArraysCommand() override;

		void reset() override;

		bool canMergeCommand(BatchCommand* cmd) override;
		void mergeCommand(BatchCommand* cmd) override;

		void appendDataToBuffer(std::vector<SpriteVertex>& bufferVertices, std::vector<GLuint>& bufferIndices) override;

		std::vector<SpriteVertex> vertices;

	protected:
		void doDispatch() override;
		GLenum mDrawMode = GL_TRIANGLES;

		GLsizei drawNumVertices = 0;
		GLint drawFirstVertex = 0;
	};

	class ClearCommand : public BatchCommand
	{
	public:
		VertexType vertexType() const override { return VertexType::NoVertex; }

		ClearCommand();
		~ClearCommand() override;

		void reset() override;

		bool canMergeCommand(BatchCommand* cmd) override;
		void mergeCommand(BatchCommand* cmd) override;

		GLbitfield mask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT;

	protected:
		void doDispatch() override;		
	};
}
