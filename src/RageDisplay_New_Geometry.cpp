
#include <global.h>

#include <RageDisplay_New_Geometry.h>

RageCompiledGeometryNew::RageCompiledGeometryNew()
{
}

RageCompiledGeometryNew::~RageCompiledGeometryNew()
{
	deallocateBuffers();
}

void RageCompiledGeometryNew::Allocate(const std::vector<msMesh>& meshes)
{
	deallocateBuffers();
	allocateBuffers();

	Change(meshes);
}

void RageCompiledGeometryNew::Change(const std::vector<msMesh>& meshes)
{
	mVBOData.clear();
	mVBOData.resize(GetTotalVertices());
	mIBOData.clear();
	mIBOData.resize(GetTotalTriangles() * 3);

	if (mVBOData.empty() || mIBOData.empty())
	{
		return;
	}

	for (auto meshI = 0; meshI < meshes.size(); ++meshI)
	{
		auto& mesh = meshes[meshI];
		auto& meshInfo = m_vMeshInfo[meshI];

		for (auto vi = 0; vi < mesh.Vertices.size(); ++vi)
		{
			const auto vo = meshInfo.iVertexStart + vi;
			const auto& v = mesh.Vertices[vi];
			mVBOData[vo] = {
				v.p,
				v.n,
				v.t,
				v.TextureMatrixScale
			};
		}

		for (auto ti = 0; ti < mesh.Triangles.size(); ++ti)
		{
			auto& tri = mesh.Triangles[ti];
			for (auto tvi = 0; tvi < 3; ++tvi)
			{
				auto vi = meshInfo.iVertexStart + tri.nVertexIndices[tvi];
				auto x = static_cast<decltype(mIBOData)::size_type>(((meshInfo.iTriangleStart + ti) * 3) + tvi);
				mIBOData[x] = vi;
			}
		}
	}
	upload();
}

void RageCompiledGeometryNew::Draw(int meshIndex) const
{
	bindVAO();

	// TODO: Ugly, but necessary for now
	// RageDisplay_New should have set up most uniforms, as needed for sprites
	// Update that to be correct for model rendering
	GLint prog = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &prog);
	auto& meshInfo = m_vMeshInfo[meshIndex];

	// TODO: Was going to set these before calling here,
	//       but didn't have access to meshInfo
	glUniform1i(glGetUniformLocation(prog, "vertexColourEnabled"), false);
	glUniform1i(glGetUniformLocation(prog, "textureMatrixScaleEnabled"), meshInfo.m_bNeedsTextureMatrixScale);

	glDrawElements(GL_TRIANGLES, meshInfo.iTriangleCount * 3, GL_UNSIGNED_INT, reinterpret_cast<const void*>(meshInfo.iTriangleStart * 3));

	unbindVAO();
}

void RageCompiledGeometryNew::bindVAO() const
{
	glBindVertexArray(mVAO);
}

void RageCompiledGeometryNew::unbindVAO() const
{
	glBindVertexArray(0);
}

void RageCompiledGeometryNew::contextLost()
{
	deallocateBuffers();
	allocateBuffers();
	upload();
}

void RageCompiledGeometryNew::allocateBuffers()
{
	glGenVertexArrays(1, &mVAO);
	glGenBuffers(1, &mVBO);
	glGenBuffers(1, &mIBO);
}

void RageCompiledGeometryNew::deallocateBuffers()
{
	if (mVBO)
	{
		glDeleteBuffers(1, &mVBO);
		mVBO = 0;
	}
	if (mIBO)
	{
		glDeleteBuffers(1, &mIBO);
		mIBO = 0;
	}
	if (mVAO)
	{
		glDeleteVertexArrays(1, &mVAO);
		mVAO = 0;
	}
}

void RageCompiledGeometryNew::upload()
{
	if (mVBOData.empty() || mIBOData.empty())
	{
		return;
  }

	bindVAO();
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mVBOData.size() * sizeof(Vertex), mVBOData.data(), GL_STATIC_DRAW);

	// TODO: Copy of RageDisplay_New::InitVertexAttribs
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, p)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, n)));
	glEnableVertexAttribArray(1);
	/*glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, c)));
	glEnableVertexAttribArray(2);*/
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, t)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), reinterpret_cast<const void*>(offsetof(Vertex, ts)));
	glEnableVertexAttribArray(4);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mIBOData.size() * sizeof(GLuint), mIBOData.data(), GL_STATIC_DRAW);

	unbindVAO();
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
