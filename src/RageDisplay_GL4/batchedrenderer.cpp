#include <global.h>

#include "batchedrenderer.h"

namespace RageDisplay_GL4
{

	BatchedRenderer::BatchedRenderer()
	{
	}
	BatchedRenderer::~BatchedRenderer()
	{
		glDeleteBuffers(1, &mSpriteVBO);
		glDeleteBuffers(1, &mSpriteIBO);
		glDeleteVertexArrays(1, &mSpriteVAO);
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
		// Note that commands should not alter opengl state, or contain their
		// own buffers - See mSpriteVAO and friends. Really these pools are just
		// to avoid allocating commands every single time, to save a little cpu overhead.
		//
	  // Frame clear - 1 for BeginFrame, but during gameplay, 1 per arrow
	  // TODO: Remove the ridiculous depth clear between arrows
		for (auto i = 0; i < 50; ++i) returnCommand(Pool::clear, std::make_shared<ClearCommand>());

		// Uncommon draws such as line strips
		for (auto i = 0; i < 10; ++i) returnCommand(Pool::sprite_tri_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLES));
		for (auto i = 0; i < 10; ++i) returnCommand(Pool::sprite_trifan_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLE_FAN));
		for (auto i = 0; i < 10; ++i) returnCommand(Pool::sprite_tristrip_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_TRIANGLE_STRIP));
		for (auto i = 0; i < 10; ++i) returnCommand(Pool::sprite_linestrip_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_LINES));
		for (auto i = 0; i < 10; ++i) returnCommand(Pool::sprite_points_arrays, std::make_shared<SpriteVertexDrawArraysCommand>(GL_POINTS));

		// Quads - A lot of these
		for (auto i = 0; i < 100; ++i) returnCommand(Pool::sprite_tri_elements, std::make_shared<SpriteVertexDrawElementsCommand>(GL_TRIANGLES));
		// TODO:
		// Compiled Geometry - Maybe a lot of these? Just how many notes can someone reasonably have on the screen at a time?
		// for (auto i = 0; i < 100; ++i) returnCommand(Pool::compiled_geometry, std::make_shared<CompiledGeometryDrawCommand>());

		glGenVertexArrays(1, &mSpriteVAO);
		glBindVertexArray(mSpriteVAO);
		glGenBuffers(1, &mSpriteVBO);
		glBindBuffer(GL_ARRAY_BUFFER, mSpriteVBO);
		ShaderProgram::configureVertexAttributesForSpriteRender();
		glGenBuffers(1, &mSpriteIBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mSpriteIBO);
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
		if( mMergeCommandsBeforeEndFrame && !commandQueue.empty() )
		{
			// To avoid copying state, check if the command can be merged into the end of the last one
			// prior to pushing it into the command queue.
			auto& previousEntry = commandQueue.back();
			if (mCurrentRenderInstance < ShaderProgram::MaxRenderInstances - 1)
			{
				if (previousEntry.command->canMergeCommand(command.get()))
				{
					if (mEnableMultiInstanceRendering)
					{
						if (previousEntry.command->state.equivalent(currentState, false))
						{
							mCurrentRenderInstance++; // Render instance of the being-merged command
							command->setRenderInstance(mCurrentRenderInstance);
							previousEntry.command->state.setRenderInstanceDataFrom(currentState, mCurrentRenderInstance);
							previousEntry.command->mergeCommand(command.get());
							returnCommand(p, command);
							return;
						}
					}
					else if (previousEntry.command->state.equivalent(currentState, true))
					{
						previousEntry.command->mergeCommand(command.get());
						returnCommand(p, command);
						return;
					}
				}
			}
		}
		mCurrentRenderInstance = 0;

		// Command couldn't be merged into the previous one
		// Copy the entirety of the current state over, and
		// add it as a distinct draw command
		setCommandState(command);
		auto entry = CommandQueueEntry{ p, command };
			commandQueue.emplace_back(std::move(entry));
	}

	void BatchedRenderer::uploadSpriteBuffers()
	{
		// Run through the queue, and upload any data compatible with the big boi
		// Also update any draw elements commands to reference the correct location
		// in said buffer
		mSpriteVerticesPreviousSize = mSpriteVertices.size();
		mSpriteElementsPreviousSize = mSpriteElements.size();
		mSpriteVertices.clear();
		mSpriteElements.clear();

		for (auto& entry : commandQueue)
		{
			if (entry.command->vertexType() != BatchCommand::VertexType::Sprite)
			{
			  // Doesn't go in this buffer
				continue;
			}

			entry.command->appendDataToBuffer(mSpriteVertices, mSpriteElements);
		}

		if (mSpriteVertices.empty() || mSpriteElements.empty())
		{
			return;
		}

		if (mSpriteVertices.size() > mSpriteVerticesPreviousSize)
		{
			// TODO: Better storage allocation without the copy
			glBufferData(GL_ARRAY_BUFFER, mSpriteVertices.size() * sizeof(SpriteVertex), mSpriteVertices.data(), GL_STREAM_DRAW);
		}
		else
		{
			auto mapped = reinterpret_cast<SpriteVertex*>(
				glMapBufferRange(GL_ARRAY_BUFFER, 0, mSpriteVertices.size() * sizeof(SpriteVertex),
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
				);
			if (mapped)
			{
				std::memcpy(mapped, mSpriteVertices.data(), mSpriteVertices.size() * sizeof(SpriteVertex));
			}
			glUnmapBuffer(GL_ARRAY_BUFFER);
		}
		if (mSpriteElements.size() > mSpriteElementsPreviousSize)
		{
			// TODO: Better storage allocation without the copy
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, mSpriteElements.size() * sizeof(GLuint), mSpriteElements.data(), GL_STREAM_DRAW);
		}
		else
		{
			auto mapped = reinterpret_cast<GLuint*>(
				glMapBufferRange(GL_ELEMENT_ARRAY_BUFFER, 0, mSpriteElements.size() * sizeof(GLuint),
					GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT)
				);
			if (mapped)
			{
				std::memcpy(mapped, mSpriteElements.data(), mSpriteElements.size() * sizeof(GLuint));
			}
			glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
		}
	}

	void BatchedRenderer::mergeCommandsInQueue()
	{
		// Merge commands into as few draw call as possible
		std::vector<CommandQueueEntry> mergedCommandQueue;
		auto renderInstance = 0;
		while (!commandQueue.empty())
		{
			CommandQueueEntry entry = commandQueue.front();
			if (commandQueue.size() > 1 && renderInstance < ShaderProgram::MaxRenderInstances - 1)
			{
				// Merge the next N commands if compatible, up to the limit of ShaderProgram::MaxRenderInstances
				auto nextEntry = commandQueue[1];

				if (entry.command->canMergeCommand(nextEntry.command.get()))
				{
					if (mEnableMultiInstanceRendering)
					{
						if (entry.command->state.equivalent(nextEntry.command->state, false))
						{
							renderInstance++; // Render instance of the being-merged command
							nextEntry.command->setRenderInstance(renderInstance);
							entry.command->state.setRenderInstanceDataFrom(nextEntry.command->state, renderInstance);
							entry.command->mergeCommand(nextEntry.command.get());
							commandQueue.erase(commandQueue.begin() + 1);
							returnCommand(nextEntry.p, nextEntry.command);
							continue;
						}
					}
					else
					{
						if (entry.command->state.equivalent(nextEntry.command->state, true))
						{
							entry.command->mergeCommand(nextEntry.command.get());
							commandQueue.erase(commandQueue.begin() + 1);
							returnCommand(nextEntry.p, nextEntry.command);
							continue;
						}
					}
				}
			}

			renderInstance = 0;
			mergedCommandQueue.push_back(entry);
			commandQueue.erase(commandQueue.begin());
		}
		commandQueue = std::move(mergedCommandQueue);
	}

	void BatchedRenderer::flushCommandQueue()
	{
		if (commandQueue.empty()) return;


		if (!mMergeCommandsBeforeEndFrame)
		{
			mergeCommandsInQueue();
		}

		// Populate the draw buffers
		// Data is packed into singular large buffers
		// TODO: For now - Is it ever possible that a buffer grows too large?
		// Bind the draw VAO
		GLuint currentVAO = mSpriteVAO;
		glBindVertexArray(mSpriteVAO);
		uploadSpriteBuffers();

		while (!commandQueue.empty())
		{
			CommandQueueEntry entry = commandQueue.front();
			switch (entry.command->vertexType())
			{
			case BatchCommand::VertexType::Sprite:
				if (currentVAO != mSpriteVAO)
				{
					glBindVertexArray(mSpriteVAO);
					currentVAO = mSpriteVAO;
				}
				break;
			case BatchCommand::VertexType::Compiled:
				// TODO
				/*if (currentVAO != mCompiledVAO)
				{
					glBindVertexArray(mCompiledVAO);
					currentVAO = mCompiledVAO;
				}*/
				break;
			default:
				break;
			}

			entry.command->dispatch(gpuState);
			// entry.command->dispatch();

			// We have changed state to the command's
			// Leave the command's state undeterminate until it is next queued
			gpuState = std::move(entry.command->state);

			if (mFlushOnDispatch)
			{
				glFlush();
			}

			commandQueue.erase(commandQueue.begin());
			returnCommand(entry.p, entry.command);
		}
	}

	void BatchedRenderer::updateGPUState(bool full)
	{
		if (full)
		{
			currentState.updateGPUState();
		}
		else
		{
			currentState.updateGPUState(gpuState);
		}
		gpuState = currentState;
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
			x->vertices = std::move(vertices);
			x->indices = std::move(elements);
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
			x->vertices = std::move(vertices);
			x->indices = std::move(elements);
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
			x->vertices = std::move(vertices);
			x->indices = std::move(elements);
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
