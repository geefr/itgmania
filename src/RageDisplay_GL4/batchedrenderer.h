#pragma once

#include <RageDisplay.h>

#include <GL/glew.h>

#include "gl4types.h"
#include "gl4state.h"
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
    void flushCommandQueue();

		/// Update the current GL state - Will be added
		/// to each command when they are pushed onto the queue
		State copyState() const { return currentState; }
		State& mutState() { return currentState; }
		const State& constState() const { return currentState; }
		/// Immediately sync the current state to the gpu
		void updateGPUState(bool full);

		/// Push commands onto the queue
		void clear();
		void clearDepthBuffer();

		void drawQuads(const RageSpriteVertex v[], int numVerts);
		void drawQuadStrip(const RageSpriteVertex v[], int numVerts);
		void drawTriangleFan(const RageSpriteVertex v[], int numVerts);
		void drawTriangleStrip(const RageSpriteVertex v[], int numVerts);
		void drawTriangles(const RageSpriteVertex v[], int numVerts);
		void drawLinestrip(const RageSpriteVertex v[], int numVerts);
		void drawPoints(const RageSpriteVertex v[], int numVerts);
		void drawSymmetricQuadStrip(const RageSpriteVertex v[], int numVerts);

		/// CompiledGeometry draws are batched across meshes
		/// if possible, but cannot be batched with primitive
		/// draws, or across multiple different models
		// void drawCompiledGeometry();

  private:
	  enum class Pool
	  {
			clear,

			sprite_tri_arrays,
			sprite_trifan_arrays,
			sprite_tristrip_arrays,
			sprite_linestrip_arrays,
			sprite_points_arrays,

			sprite_tri_elements,
		};
    // Get a batch from the pool, return empty ptr if pool is empty
	  // In this case the caller should either flush batches and retry
	  // or create a batch as needed (optionally giving it to the pool for later re-use)
	  std::shared_ptr<BatchCommand> fishCommand(Pool p);
	  void returnCommand(Pool p, std::shared_ptr<BatchCommand> b);
	  void setCommandState(std::shared_ptr<BatchCommand> b);
		std::map<Pool, std::vector<std::shared_ptr<BatchCommand>>> commandPools;

		struct CommandQueueEntry
		{
			Pool p;
			std::shared_ptr<BatchCommand> command;
		};
    std::vector<CommandQueueEntry> commandQueue;
	  // Queue a command - p is the pool to return the command to after
	  // and does not strictly need to be the same pool it came from
	  // e.g. For destroying commands, if that ever happens
		void queueCommand(Pool p, std::shared_ptr<BatchCommand> command);

	  // The temporary state used during draw commands, is copied
	  // to each command when queued
		State currentState;
		// The state on the GPU - Only modified when a command
		// is dispatched
		State gpuState;

		bool mFlushOnDispatch = false;

		GLuint mSpriteVAO = 0;
		GLuint mSpriteVBO = 0;
		GLuint mSpriteIBO = 0;

		std::vector<SpriteVertex> mSpriteVertices;
		GLuint mSpriteVerticesPreviousSize = 0;
		std::vector<GLuint> mSpriteElements;
		GLuint mSpriteElementsPreviousSize = 0;
		void uploadSpriteBuffers();
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
