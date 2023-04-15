#include <global.h>

#include "batchedrenderer.h"

namespace RageDisplay_GL4
{

	BatchedRenderer::BatchedRenderer()
		: currentState(new State())
	{
	}
	BatchedRenderer::~BatchedRenderer()
	{
	}

	void BatchedRenderer::init()
	{
		// Always enabled
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		auto s = state();
		// https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glLight.xml
		for (auto i = 1; i < ShaderProgram::MaxLights; ++i)
		{
			auto& l = s.uniformBlockLights[i];
			l.enabled = false;
			l.ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
			l.diffuse = { 0.0f, 0.0f, 0.0f, 1.0f };
			l.specular = { 0.0f, 0.0f, 0.0f, 1.0f };
			l.position = { 0.0f, 0.0f, 1.0f , 0.0f };
		}

		auto& light0 = s.uniformBlockLights[0];
		light0.enabled = true;
		light0.ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		light0.diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
		light0.specular = { 1.0f, 1.0f, 1.0f, 1.0f };
		light0.position = { 0.0f, 0.0f, 1.0f, 0.0f };
		setState(s);

		currentState->updateGPUState();

		// Initialise command pools
		// The pools start with a reasonable number of commands
		// and may expand later if needed to contain 1 frame's worth of commands
		// TODO: Using a separate command with its own VAO/VBO/IBO for everything
		//       seems wasteful. Perhaps it's better to just have one per
		//       vertex type, since we're not caching vertices across frames
		//       yet anyway?
	  // TODO: I assume the limit is too high to worry, but at some point we
	  //       will run out of buffers. Perhaps the better approach is to
	  //       force a flush if the pool runs out, but also to detect
	  //       whether commands can be merged on the fly, and flush
	  //       the queue up for 0..n-1 as soon as a non-mergable command is found.

	  // Frame clear - 1 for BeginFrame, but during gameplay, 1 per arrow
	  // TODO: Remove the ridiculous depth clear between arrows
		for (auto i = 0; i < 50; ++i) returnCommand(Pool::clear, std::make_shared<ClearCommand>());

		// Uncommon draws such as line strips
		for (auto i = 0; i < 5; ++i) returnCommand(Pool::sprite_tri_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLES));
		for (auto i = 0; i < 5; ++i) returnCommand(Pool::sprite_trifan_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLE_FAN));
		for (auto i = 0; i < 5; ++i) returnCommand(Pool::sprite_tristrip_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLE_STRIP));
		for (auto i = 0; i < 5; ++i) returnCommand(Pool::sprite_linestrip_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_LINES));
		for (auto i = 0; i < 5; ++i) returnCommand(Pool::sprite_points_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_POINTS));

		// Draw quads - HOTPATH
		for (auto i = 0; i < 100; ++i) returnCommand(Pool::sprite_tri_elements, std::make_shared<SpriteVertexDrawElementsCommand>(GL_TRIANGLES));
	}

	std::shared_ptr<BatchCommand> BatchedRenderer::fishCommand(Pool p)
	{
		auto& pool = commandPools[p];
		if (pool.empty()) return {};
		auto b = std::move(pool.front());
		pool.erase(pool.begin());
		return b;
	}

	void BatchedRenderer::setCommandState(std::shared_ptr<BatchCommand> b)
	{
		b->state = *currentState;
	}

	void BatchedRenderer::returnCommand(Pool p, std::shared_ptr<BatchCommand> b)
	{
		b->reset();
		commandPools[p].push_back(b);
	}

	void BatchedRenderer::queueCommand(Pool p, std::shared_ptr<BatchCommand> command)
	{
		setCommandState(command);
		auto entry = CommandQueueEntry{ p, command };

		if (!mEagerFlush || commandQueue.empty())
		{
			commandQueue.emplace_back(std::move(entry));
			return;
		}

		// Eager flush
		auto previousEntry = commandQueue.back();
		if( previousEntry.command->canMergeCommand(entry.command.get()) &&
				previousEntry.command->state.equivalent(entry.command->state) )
		{
		  // Make the next command a part of the last one
			previousEntry.command->mergeCommand(entry.command.get());
			returnCommand(entry.p, entry.command);
		}
		else
		{
		  // Flush what we had so far
		  if(previousState)
		  {
				previousEntry.command->dispatch(*previousState);
			  *previousState = previousEntry.command->state;
			}
			else
			{
				previousEntry.command->dispatch();
				previousState.reset(new State(previousEntry.command->state));
			}

		  if (mFlushOnDispatch)
		  {
			  glFlush();
			}

			commandQueue.pop_back();
			returnCommand(previousEntry.p, previousEntry.command);

	    // Queue the new command
			commandQueue.emplace_back(std::move(entry));
		}
	}

	void BatchedRenderer::flushCommandQueue()
	{
		if (commandQueue.empty()) return;

		while (!commandQueue.empty())
		{
			auto entry = commandQueue.front();
			if (commandQueue.size() > 1)
			{
				auto nextEntry = commandQueue[1];

				if (entry.command->canMergeCommand(nextEntry.command.get()))
				{
					if (entry.command->state.equivalent(nextEntry.command->state))
					{
						entry.command->mergeCommand(nextEntry.command.get());
						commandQueue.erase(commandQueue.begin() + 1);
						returnCommand(nextEntry.p, nextEntry.command);
						continue;
					}
				}
			}

			if (previousState)
			{
				entry.command->dispatch(*previousState);
				*previousState = entry.command->state;
			}
			else
			{
				entry.command->dispatch();
				previousState.reset(new State(entry.command->state));
			}

			if (mFlushOnDispatch)
			{
				glFlush();
			}

			commandQueue.erase(commandQueue.begin());
			returnCommand(entry.p, entry.command);
		}
	}

	void BatchedRenderer::clear()
	{
		auto s = state();
		s.globalState.clearColour = { 0.0f, 0.0f, 0.0f, 0.0f };
		s.globalState.depthWriteEnabled = true;
		setState(s);

		auto command = fishCommand(Pool::clear);
		if (!command)
		{
			command = std::make_shared<ClearCommand>();
		}
		queueCommand(Pool::clear, command);
	}

	void BatchedRenderer::clearDepthBuffer()
	{
		auto s = state();
		auto depthWriteWasEnabled = s.globalState.depthWriteEnabled;
		s.globalState.depthWriteEnabled = true;
		setState(s);

		auto command = fishCommand(Pool::clear);
		if (!command)
		{
			command = std::make_shared<ClearCommand>();
		}
		std::dynamic_pointer_cast<ClearCommand>(command)->mask = GL_DEPTH_BUFFER_BIT;
		queueCommand(Pool::clear, command);

		s = state();
		s.globalState.depthWriteEnabled = depthWriteWasEnabled;
		setState(s);
	}

	void BatchedRenderer::setState(const State& state)
	{
		*currentState = state;
	}

	void BatchedRenderer::drawQuads(const RageSpriteVertex v[], int numVerts)
	{
		if (numVerts < 4) return;

		auto numTriVerts = (numVerts / 4) * 6;
		std::vector<GLuint> elements;
		elements.reserve(numTriVerts);
		for (auto i = 0; i < numVerts; i += 4)
		{
			elements.emplace_back(i + 0);
			elements.emplace_back(i + 1);
			elements.emplace_back(i + 2);
			elements.emplace_back(i + 2);
			elements.emplace_back(i + 3);
			elements.emplace_back(i + 0);
		}

		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_tri_elements);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawElementsCommand>(GL_TRIANGLES);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawElementsCommand>(command))
		{
			x->addDraw(std::move(vertices), std::move(elements));
		}
		queueCommand(Pool::sprite_tri_elements, command);
	}

	void BatchedRenderer::drawQuadStrip(const RageSpriteVertex v[], int numVerts)
	{
		// 0123 -> 102 123
		// 2345 -> 324 345
		std::vector<GLuint> elements;
		elements.reserve(numVerts - 2);
		for (auto i = 0; i < numVerts - 2; i += 2)
		{
			elements.emplace_back(i + 1);
			elements.emplace_back(i + 0);
			elements.emplace_back(i + 2);
			elements.emplace_back(i + 1);
			elements.emplace_back(i + 2);
			elements.emplace_back(i + 3);
		}

		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_tri_elements);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawElementsCommand>(GL_TRIANGLES);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawElementsCommand>(command))
		{
			x->addDraw(std::move(vertices), std::move(elements));
		}
		queueCommand(Pool::sprite_tri_elements, command);
	}

	void BatchedRenderer::drawTriangleFan(const RageSpriteVertex v[], int numVerts)
	{
		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_trifan_arrays);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLE_FAN);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawArraysCommand>(command))
		{
			x->vertices = std::move(vertices);
		}
		queueCommand(Pool::sprite_trifan_arrays, command);
	}

	void BatchedRenderer::drawTriangleStrip(const RageSpriteVertex v[], int numVerts)
	{
		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_tristrip_arrays);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLE_STRIP);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawArraysCommand>(command))
		{
			x->vertices = std::move(vertices);
		}
		queueCommand(Pool::sprite_tristrip_arrays, command);
	}

	void BatchedRenderer::drawTriangles(const RageSpriteVertex v[], int numVerts)
	{
		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_tri_arrays);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLES);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawArraysCommand>(command))
		{
			x->vertices = std::move(vertices);
		}
		queueCommand(Pool::sprite_tri_arrays, command);
	}

	void BatchedRenderer::drawLinestrip(const RageSpriteVertex v[], int numVerts)
	{
		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_linestrip_arrays);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawArraysCommand>(GL_LINE_STRIP);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawArraysCommand>(command))
		{
			x->vertices = std::move(vertices);
		}
		queueCommand(Pool::sprite_linestrip_arrays, command);
	}

	void BatchedRenderer::drawPoints(const RageSpriteVertex v[], int numVerts)
	{
		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_points_arrays);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawArraysCommand>(GL_POINTS);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawArraysCommand>(command))
		{
			x->vertices = std::move(vertices);
		}
		queueCommand(Pool::sprite_points_arrays, command);
	}

	void BatchedRenderer::drawSymmetricQuadStrip(const RageSpriteVertex v[], int numVerts)
	{
		// Thanks RageDisplay_Legacy - No time to work this out,
		// ported index buffer logic directly
		int numPieces = (numVerts - 3) / 3;
		int numTriangles = numPieces * 4;
		std::vector<GLuint> elements;
		elements.reserve(numTriangles * 3);
		for (auto i = 0; i < numPieces; ++i)
		{
			// { 1, 3, 0 } { 1, 4, 3 } { 1, 5, 4 } { 1, 2, 5 }
			elements.emplace_back(i * 3 + 1);
			elements.emplace_back(i * 3 + 3);
			elements.emplace_back(i * 3 + 0);
			elements.emplace_back(i * 3 + 1);
			elements.emplace_back(i * 3 + 4);
			elements.emplace_back(i * 3 + 3);
			elements.emplace_back(i * 3 + 1);
			elements.emplace_back(i * 3 + 5);
			elements.emplace_back(i * 3 + 4);
			elements.emplace_back(i * 3 + 1);
			elements.emplace_back(i * 3 + 2);
			elements.emplace_back(i * 3 + 5);
		}

		// Note RageVColor is bgra in memory
		std::vector<SpriteVertex> vertices;
		vertices.reserve(numVerts);
		for (auto i = 0; i < numVerts; ++i)
		{
			auto& vert = v[i];
			SpriteVertex sv;
			sv.p = vert.p;
			sv.n = vert.n;
			sv.c = RageVector4(
				static_cast<float>(vert.c.r) / 255.0f,
				static_cast<float>(vert.c.g) / 255.0f,
				static_cast<float>(vert.c.b) / 255.0f,
				static_cast<float>(vert.c.a) / 255.0f
			);
			sv.t = vert.t;
			vertices.emplace_back(std::move(sv));
		}

		auto command = fishCommand(Pool::sprite_tri_elements);
		if (!command)
		{
			// TODO: For now, let the pools grow
			command = std::make_shared<SpriteVertexDrawElementsCommand>(GL_TRIANGLES);
		}
		if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawElementsCommand>(command))
		{
			x->addDraw(std::move(vertices), std::move(elements));
		}
		queueCommand(Pool::sprite_tri_elements, command);
	}
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
