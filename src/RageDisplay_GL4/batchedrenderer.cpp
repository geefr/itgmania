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

		for (auto i = 0; i < 100; ++i)
		{
			freeBatchPool.emplace_back(new SpriteVertexBatch(GL_TRIANGLES, {}));
		}
	}

	void BatchedRenderer::flushBatches()
	{
		if (batches.empty()) return;

		while (!batches.empty())
		{
			auto& command = batches.front();
			if (batches.size() > 1)
			{
				auto& nextCommand = batches[1];

				// TODO: If overall batch types are compatible
				//       i.e. overall type, GL_TRIANGLES, etc.
				// command.mergeCommand(nextCommand);
				// batches.erase(1);
				// continue;

				auto spriteCommand = dynamic_cast<SpriteVertexBatch*>(command.get());
				auto spriteNextCommand = dynamic_cast<SpriteVertexBatch*>(nextCommand.get());
				if (spriteCommand->mState.equivalent(spriteNextCommand->mState))
				{
					spriteCommand->addDraw(spriteNextCommand->vertices(), spriteNextCommand->indices());
					freeBatchPool.emplace_back(std::move(spriteNextCommand));
					batches.erase(batches.begin() + 1);
					continue;
				}
			}

			if (auto x = dynamic_cast<SpriteVertexBatch*>(command.get()))
			{
				if (previousState)
				{
					x->flush(*previousState);
				}
				else
				{
					x->flush();
				}
				previousState.reset(new State(x->mState));
			}

			freeBatchPool.push_back(std::dynamic_pointer_cast<SpriteVertexBatch>(command));
			batches.erase(batches.begin());
		}
	}

	void BatchedRenderer::clear()
	{
		flushBatches();

		currentState->globalState.depthWriteEnabled = true;
		if( previousState )
			currentState->updateGPUState(*previousState);
		else
			currentState->updateGPUState();

		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		previousState.reset(new State(*currentState));
	}

	void BatchedRenderer::clearDepthBuffer()
	{
		flushBatches();

		bool depthWriteWasEnabled = currentState->globalState.depthWriteEnabled;
		currentState->globalState.depthWriteEnabled = true;
		if (previousState)
			currentState->updateGPUState(*previousState);
		else
			currentState->updateGPUState();

		glClear(GL_DEPTH_BUFFER_BIT);

		previousState.reset(new State(*currentState));
		currentState->globalState.depthWriteEnabled = depthWriteWasEnabled;
	}

	void BatchedRenderer::setState(const State& state)
	{
		// TODO: Maybe an operator=, that would be cool
		currentState.reset(new State(state));
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
		vertices.reserve(numTriVerts);
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

		// TODO: Yeah, recreating draw commands all the time sucks, we should use pools
		std::shared_ptr<SpriteVertexBatch> batch;
		if (!freeBatchPool.empty())
		{
			batch = freeBatchPool.front();
			batch->clear();
			batch->mState = *currentState;
			freeBatchPool.erase(freeBatchPool.begin());
		}
		else
		{
			batch = std::make_unique<SpriteVertexBatch>(GL_TRIANGLES, *currentState);
		}

		batch->addDraw(std::move(vertices), std::move(elements));
		batches.emplace_back(std::move(batch));
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
