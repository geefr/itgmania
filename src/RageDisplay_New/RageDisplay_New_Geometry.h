#pragma once

#include <RageDisplay.h>

#include <GL/glew.h>

class RageCompiledGeometryNew final : public RageCompiledGeometry
{
public:
	RageCompiledGeometryNew();
	~RageCompiledGeometryNew() override;
	void Allocate(const std::vector<msMesh>& meshes) override;
	void Change(const std::vector<msMesh>& meshes) override;
	void Draw(int meshIndex) const override;

	void bindVAO() const;
	void unbindVAO() const;

	void contextLost();
private:
  void allocateBuffers();
  void deallocateBuffers();
	void upload();

	struct Vertex
	{
	  RageVector3 p; // position
	  RageVector3 n; // normal
	  // RageVColor c; // Vertex colour - Not applicable for models
	  RageVector2 t; // texcoord
	  RageVector2 ts; // texture matrix scale
	};
	std::vector<Vertex> mVBOData;
	std::vector<GLuint> mIBOData;

	GLuint mVAO = 0;
	GLuint mVBO = 0;

	//  TODO: nononoo this is wrong - One big VBO, multimple smaller IBOs, or somehow mapping to the mesh indexes for the draw
	GLuint mIBO = 0;
};

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
