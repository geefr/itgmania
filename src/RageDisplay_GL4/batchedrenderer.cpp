#include <global.h>

#include "batchedrenderer.h"

namespace RageDisplay_GL4
{

	BatchedRenderer::BatchedRenderer()
	{
	}
	BatchedRenderer::~BatchedRenderer()
	{
		glDeleteBuffers(1, &mBigBufferVBO);
		glDeleteBuffers(1, &mBigBufferVAO);
		glDeleteVertexArrays(1, &mBigBufferVAO);
	}

	void BatchedRenderer::init()
	{
		// Always enabled
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_BLEND);

		auto& s = mutState();
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

		currentState.updateGPUState();
		gpuState = currentState;

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
		for (auto i = 0; i < 50; ++i) returnCommand(Pool::sprite_tri_elements, std::make_shared<SpriteVertexDrawElementsCommand>(GL_TRIANGLES));
		for (auto i = 0; i < 100; ++i) returnCommand(Pool::sprite_tri_elements_big_buffer, std::make_shared<SpriteVertexDrawElementsFromOneBigBufferCommand>(GL_TRIANGLES));

		glGenBuffers(1, &mBigBufferVBO);
		glGenBuffers(1, &mBigBufferIBO);
		glGenVertexArrays(1, &mBigBufferVAO);
		glBindVertexArray(mBigBufferVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mBigBufferVBO);
		ShaderProgram::configureVertexAttributesForSpriteRender();
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBigBufferIBO);
		glBindVertexArray(0);
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
		b->state = currentState;
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
			previousEntry.command->dispatch(gpuState);
			// We have changed state to the command's
			// Leave the command's state undeterminate until it is next queued
			gpuState = std::move(previousEntry.command->state);
		  /*if(previousState)
		  {
				previousEntry.command->dispatch(*previousState);
			  *previousState = previousEntry.command->state;
			}*/
			/*else
			{
				previousEntry.command->dispatch();
				previousState.reset(new State(previousEntry.command->state));
			}*/

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

	void BatchedRenderer::uploadBigBufferData()
	{
		// Run through the queue, and upload any data compatible with the big boi
		// Also update any draw elements commands to reference the correct location
		// in said buffer
		mBigBufferVerticesPreviousSize = mBigBufferVertices.size();
		mBigBufferElementsPreviousSize = mBigBufferElements.size();
		mBigBufferVertices.clear();
		mBigBufferElements.clear();

		for (auto& entry : commandQueue)
		{
			if( auto x = std::dynamic_pointer_cast<SpriteVertexDrawElementsFromOneBigBufferCommand>(entry.command) )
			{
				if( x->drawNumIndices == 0 ) continue;

				auto vertexStart = mBigBufferVertices.size();
				for( auto& i : x->indices ) i += vertexStart;

				mBigBufferVertices.insert(mBigBufferVertices.end(),
					std::make_move_iterator(x->vertices.begin()),
					std::make_move_iterator(x->vertices.end())
				);
				x->vertices = {};

				x->indexBufferOffset = mBigBufferElements.size();
				mBigBufferElements.insert(mBigBufferElements.end(),
					std::make_move_iterator(x->indices.begin()),
					std::make_move_iterator(x->indices.end())
				);
				x->indices = {};
			}
		}

		if (mBigBufferVertices.empty() || mBigBufferElements.empty())
		{
			return;
		}

		glBindVertexArray(mBigBufferVAO);
		glBindBuffer(GL_ARRAY_BUFFER, mBigBufferVBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mBigBufferIBO);
		if (mBigBufferVertices.size() > mBigBufferVerticesPreviousSize)
		{
			// TODO: Better storage allocation without the copy
			glBufferData(GL_ARRAY_BUFFER, mBigBufferVertices.size() * sizeof(SpriteVertex), mBigBufferVertices.data(), GL_STREAM_DRAW);
		}
		else
		{
			auto mapped = reinterpret_cast<SpriteVertex*>(
				glMapBufferRange(GL_ARRAY_BUFFER, 0, mBigBufferVertices.size() * sizeof(SpriteVertex),
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
				);
			if (mapped)
			{
				std::memcpy(mapped, mBigBufferVertices.data(), mBigBufferVertices.size() * sizeof(SpriteVertex));
			}
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}
		if (mBigBufferElements.size() > mBigBufferElementsPreviousSize)
		{
			// TODO: Better storage allocation without the copy
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, mBigBufferElements.size() * sizeof(GLuint), mBigBufferElements.data(), GL_STREAM_DRAW);
		}
		else
		{
			auto mapped = reinterpret_cast<GLuint*>(
				glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, mBigBufferElements.size() * sizeof(GLuint),
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
				);
			if (mapped)
			{
				std::memcpy(mapped, mBigBufferElements.data(), mBigBufferElements.size() * sizeof(GLuint));
			}
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}
	}

	void BatchedRenderer::flushCommandQueue()
	{
		if (commandQueue.empty()) return;

		// TODO: An experiment, will need to restructure
		if (mQuadsUseBigBuffer)
		{
			uploadBigBufferData();
		}

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

			// TODO: An experiment - But will need a better way to activate VAOs and such
			//       Seems like a series of buffer classes would be in order here, to
			//       make things a lot more generic with a BatchDrawCommand::populateBuffer or similar
			if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawElementsFromOneBigBufferCommand>(entry.command))
			{
				glBindVertexArray(mBigBufferVAO);
			}

			entry.command->dispatch(gpuState);
			// We have changed state to the command's
			// Leave the command's state undeterminate until it is next queued
			gpuState = std::move(entry.command->state);
			//if (previousState)
			//{
			//	entry.command->dispatch(*previousState);
			//	*previousState = entry.command->state;
			//}
			//else
			//{
			//	entry.command->dispatch();
			//	previousState.reset(new State(entry.command->state));
			//}

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
		auto& s = mutState();
		s.globalState.clearColour = { 0.0f, 0.0f, 0.0f, 0.0f };
		s.globalState.depthWriteEnabled = true;

		auto command = fishCommand(Pool::clear);
		if (!command)
		{
			command = std::make_shared<ClearCommand>();
		}
		queueCommand(Pool::clear, command);
	}

	void BatchedRenderer::clearDepthBuffer()
	{
		auto& s = mutState();
		auto depthWriteWasEnabled = s.globalState.depthWriteEnabled;
		s.globalState.depthWriteEnabled = true;

		auto command = fishCommand(Pool::clear);
		if (!command)
		{
			command = std::make_shared<ClearCommand>();
		}
		std::dynamic_pointer_cast<ClearCommand>(command)->mask = GL_DEPTH_BUFFER_BIT;
		queueCommand(Pool::clear, command);

		s.globalState.depthWriteEnabled = depthWriteWasEnabled;
	}

	//void BatchedRenderer::setState(const State& state)
	//{
	//	*currentState = state;
	//}

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

		if (mQuadsUseBigBuffer)
		{
			auto command = fishCommand(Pool::sprite_tri_elements_big_buffer);
			if (!command)
			{
				// TODO: For now, let the pools grow
				command = std::make_shared<SpriteVertexDrawElementsFromOneBigBufferCommand>(GL_TRIANGLES);
			}
			if (auto x = std::dynamic_pointer_cast<SpriteVertexDrawElementsFromOneBigBufferCommand>(command))
			{
				x->indexBufferOffset = 0;
				x->drawNumIndices = elements.size();
				x->vertices = std::move(vertices);
				x->indices = std::move(elements);
			}
			queueCommand(Pool::sprite_tri_elements_big_buffer, command);
		}
		else
		{
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
