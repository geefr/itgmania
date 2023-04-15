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
		virtual ~BatchCommand() {};

		virtual void dispatch();
		virtual void dispatch(const State& previousState);
		virtual void reset() = 0;

		// Yes, non-const
		// But all commands _must_ be re-usable after merging, and calling reset()
		virtual bool canMergeCommand(BatchCommand* cmd) { return false; }
		virtual void mergeCommand(BatchCommand* cmd) {}

		State state;
	protected:
		BatchCommand() {};
		virtual void doDispatch() = 0;
	};

	class BatchDrawCommand : public BatchCommand
	{
	public:
		BatchDrawCommand();
		~BatchDrawCommand() override;
  protected:
		GLuint mVAO = 0;
		GLuint mVBO = 0;
		GLuint mIBO = 0;

	  /*static GLuint mVAO;
	  static GLuint mVBO;
	  static GLuint mIBO;*/
	};

	class SpriteVertexDrawElementsCommand : public BatchDrawCommand
	{
	public:
		SpriteVertexDrawElementsCommand() = delete;
		SpriteVertexDrawElementsCommand(GLenum drawMode);
		~SpriteVertexDrawElementsCommand() override;

		void reset() override;

		bool canMergeCommand(BatchCommand* cmd) override;
		void mergeCommand(BatchCommand* cmd) override;

		void addDraw(
			std::vector<SpriteVertex>&& vertices,
			std::vector<GLuint>&& indices);

		std::vector<SpriteVertex> vertices() { return mVertices; }
		std::vector<GLuint> indices() { return mIndices; }

	protected:
		void doDispatch() override;

		std::vector<SpriteVertex> mVertices;
		std::vector<GLuint> mIndices;
		GLenum mDrawMode = GL_TRIANGLES;
	};

	class SpriteVertexDrawArraysCommand : public BatchDrawCommand
	{
	public:
		SpriteVertexDrawArraysCommand() = delete;
		SpriteVertexDrawArraysCommand(GLenum drawMode);
		~SpriteVertexDrawArraysCommand() override;

		void reset() override;

		bool canMergeCommand(BatchCommand* cmd) override;
		void mergeCommand(BatchCommand* cmd) override;

		std::vector<SpriteVertex> vertices;

	protected:
		void doDispatch() override;
		GLenum mDrawMode = GL_TRIANGLES;
	};

	class ClearCommand : public BatchCommand
	{
	public:
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
