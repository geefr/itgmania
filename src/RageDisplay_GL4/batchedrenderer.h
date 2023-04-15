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
    void flushBatches();

    /// flush batches and glClear
    void clear();
    void clearDepthBuffer();

		State state() const { return *currentState; }
		void setState(const State& state);

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

  private:
    std::vector<std::shared_ptr<Batch>> batches;
		std::vector<std::shared_ptr<SpriteVertexBatch>> freeBatchPool;
		// Access to state only safe if batches.empty()
		// use setState otherwise
		std::unique_ptr<State> currentState;
		std::unique_ptr<State> previousState;
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
