#include <global.h>

#include "batchedrenderer.h"

namespace RageDisplay_GL4
{

	BatchedRenderer::BatchedRenderer()
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

		mQuadsBatch.reset(new SpriteVertexBatch(GL_TRIANGLES));
	}

	void BatchedRenderer::flushBatches()
	{
		mQuadsBatch->flush();
	}

	void BatchedRenderer::clear()
	{
		flushBatches();
		depthWrite(true);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
    void BatchedRenderer::clearDepthBuffer()
	{
		flushBatches();
		bool oldState = mDepthWriteEnabled;
		depthWrite(true);
		glClear(GL_DEPTH_BUFFER_BIT);
		depthWrite(oldState);
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

		mQuadsBatch->addDraw(std::move(vertices), std::move(elements));
	}

	void BatchedRenderer::globalStateChanged()
	{
		flushBatches();
	}

    void BatchedRenderer::lineOrPointStateChanged()
	{
		flushBatches();
	}

	//	void shaderStateChanged();

    void BatchedRenderer::textureStateChanged()
	{
		flushBatches();
	}

	void BatchedRenderer::depthWrite(bool enable)
	{
		if (enable != mDepthWriteEnabled)
		{
			globalStateChanged();
			if (enable)
				glDepthMask(GL_TRUE);
			else
				glDepthMask(GL_FALSE);
			mDepthWriteEnabled = enable;
		}
	}
	void BatchedRenderer::depthFunc(GLenum func)
	{
		if (func != mDepthFunc)
		{
			globalStateChanged();
			glDepthFunc(func);
			mDepthFunc = func;
		}
	}
	void BatchedRenderer::depthRange(float zNear, float zFar)
	{
		if (zNear != mDepthNear || zFar != mDepthFar)
		{
			globalStateChanged();
			glDepthRange(zNear, zFar);
			mDepthNear = zNear;
			mDepthFar = zFar;
		}
	}
	void BatchedRenderer::blendMode(GLenum eq, GLenum sRGB, GLenum dRGB, GLenum sA, GLenum dA)
	{
		if (eq != mBlendEq || sRGB != mBlendSourceRGB || dRGB != mBlendDestRGB || sA != mBlendSourceAlpha || dA != mBlendDestAlpha)
		{
			globalStateChanged();
			glBlendEquation(eq);
			glBlendFuncSeparate(sRGB, dRGB, sA, dA);
			mBlendEq = eq;
			mBlendSourceRGB = sRGB;
			mBlendDestRGB = dRGB;
			mBlendSourceAlpha = sA;
			mBlendDestAlpha = dA;
		}
	}
	void BatchedRenderer::cull(bool enable, GLenum face)
	{
		if (enable != mCullEnabled || face != mCullFace)
		{
			globalStateChanged();
			if (enable)
			{
				glEnable(GL_CULL_FACE);
				glCullFace(face);
			}
			else
				glDisable(GL_CULL_FACE);
			mCullEnabled = enable;
			mCullFace = face;
		}
	}
	/*
	void BatchedRenderer::alphaTest(bool enable)
	{
		if( enable != mAlphaTestEnabled )
		{
			if( enable )
			{
				mAlphaTestEnabled = true;
				// TODO: This is in the UBO
			}
		}
	}
	*/
	void BatchedRenderer::lineWidth(float w)
	{
		if( w != mLineWidth )
		{
			globalStateChanged();
			glLineWidth(w);
			mLineWidth = w;
		}
	}
	void BatchedRenderer::pointSize(float s)
	{
		if( s != mPointSize )
		{
			globalStateChanged();
			glPointSize(s);
			mPointSize = s;
		}
	}

	void BatchedRenderer::addTexture(GLuint tex, bool hasMipMaps)
	{
		mTextureState[tex] = {
			hasMipMaps
		};
	}
	bool BatchedRenderer::textureHasMipMaps(GLuint tex) const
	{
		auto it = mTextureState.find(tex);
		if( it == mTextureState.end() ) return false;
		return it->second.hasMipMaps;
	}
    void BatchedRenderer::removeTexture(GLuint tex)
	{
		textureStateChanged();
		auto it = mTextureState.find(tex);
		if( it == mTextureState.end() ) return;
		mTextureState.erase(it);
	}
  void BatchedRenderer::invalidateTexture(GLuint tex)
	{
		// Again, this is arguably a state change
		// but either the texture is already bound and just updated
		// or a different one should be bound shortly to the sampler
	}
  void BatchedRenderer::bindTexture(GLuint unit, GLuint tex)
	{
	  textureStateChanged();
		if (tex == 0) return;
		if( tex == boundTexture(unit) ) return;
		mBoundTextures[unit] = tex;
		glActiveTexture(unit);
		glBindTexture(GL_TEXTURE_2D, tex);
	}
	GLuint BatchedRenderer::boundTexture(GLuint unit) const
	{
		auto it = mBoundTextures.find(unit);
		if( it != mBoundTextures.end() ) return it->second;
		return 0;
	}
	void BatchedRenderer::textureWrap(GLuint unit, GLenum mode)
	{
		auto tex = boundTexture(unit);
		 if( mTextureState[tex].wrapMode != mode )
		{
			 textureStateChanged();
			mTextureState[tex].wrapMode = mode;
			glActiveTexture(unit);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode);
		}
	}
	void BatchedRenderer::textureFilter(GLuint unit, GLenum minFilter, GLenum magFilter)
	{
		auto tex = boundTexture(unit);
		if( mTextureState[tex].minFilter != minFilter ||
		    mTextureState[tex].magFilter != magFilter )
		{
			textureStateChanged();
			mTextureState[tex].minFilter = minFilter;
		    mTextureState[tex].magFilter = magFilter;
			glActiveTexture(unit);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		}
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
