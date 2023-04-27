#pragma once

#include <RageTypes.h>

#include <GL/glew.h>

#include <ctype.h>
#include <RageDisplay.h>

#include "gl4types.h"

namespace RageDisplay_GL4
{

/**
 * Wrapper around an OpenGL shader program, along with
 * the vertex and uniform interfaces used by RageDisplay_New
 *
 * A single interface is used for both sprite-based rendering (DrawQuad and friends)
 * and compiled geometry (character models, 3D noteskins).
 * This interface is slightly different in that models don't include vertex colours,
 * but it's not worth writing two separate shader apis.
 */
class ShaderProgram
{
public:
	/// Configure the currently bound VAO - Include vertex colours, exclude texture scaling
	static void configureVertexAttributesForSpriteRender();
	/// Configure the currently bound VAO - Exclude vertex colours, include texture scaling
	static void configureVertexAttributesForCompiledRender();

	/// Compile and link a shader program
	/// @return non-zero on success, zero on error
	static GLuint LoadShaderProgram(std::string vert, std::string frag, bool assertOnError);

	/// Compile a shader from file
	/// @return non-zero on success, zero on error
	static GLuint LoadShader(GLenum type, std::string source, bool assertOnError);

	// THERE ARE FOUR LIGHTS >:3
	static const int MaxLights = 4;
	// Also four textures =^v^=
	static const int MaxTextures = 4;
	// And four render instances at a time
	// TODO: For now - If each render only has a single texture, we could do 16
	//       or even more, depending on the max-bound-samplers limit.
	static const int MaxRenderInstances = 4;

	ShaderProgram();
	ShaderProgram(std::string vertShader, std::string fragShader);
	~ShaderProgram();

	const UniformBlockMatrices& uniformMatrices(const uint32_t renderInstance) const { return mUniformBlockMatrices[renderInstance]; }
	bool setUniformMatrices(const uint32_t renderInstance, const UniformBlockMatrices& block);

	const UniformBlockTextureSettings& uniformTextureSettings(const uint32_t renderInstance, const uint32_t index) const { return mUniformBlockTextureSettings[(renderInstance * MaxRenderInstances) + index]; }
	bool setUniformTextureSettings(const uint32_t renderInstance, const uint32_t index, const UniformBlockTextureSettings& block);

	TextureUnit uniformTextureUnit(const uint32_t renderInstance, const uint32_t index) const { return static_cast<TextureUnit>(mUniformTextureUnits[(renderInstance * MaxRenderInstances) + index]); }
	bool setUniformTextureUnit(const uint32_t renderInstance, const uint32_t index, const TextureUnit& unit);

	const UniformBlockMaterial& uniformMaterial(const uint32_t renderInstance) const { return mUniformBlockMaterial[renderInstance]; }
	bool setUniformMaterial(const uint32_t renderInstance, const UniformBlockMaterial& block);

	const UniformBlockLight& uniformLight(const uint32_t renderInstance, const uint32_t index) const { return mUniformBlockLights[(renderInstance * MaxRenderInstances) + index]; }
	bool setUniformLight(const uint32_t renderInstance, const uint32_t index, const UniformBlockLight& block);

	/// Compile the shader, configure uniform buffers
	bool init();

	/// Make the shader active
	void bind();

	/// Update any uniforms if needed
	/// And otherwise prepare to render
	void updateUniforms();

	/// Free all buffers, unload the shader, reset
	/// You may call init() again (under a new context if you like)
	/// Uniform data is reset to defaults to be safe (texture handles)
	void invalidate();

private:
	void initUniforms();

	// TODO: For now at least, separate UBOs, and no sharing across shader programs
	UniformBlockMatrices mUniformBlockMatrices[MaxRenderInstances];
	bool mUniformBlockMatricesChanged = true;
	GLuint mUniformBlockMatricesIndex = 0;
	GLuint mUniformBlockMatricesUBO = 0;

	UniformBlockTextureSettings mUniformBlockTextureSettings[MaxRenderInstances * MaxTextures];
	bool mUniformBlockTextureSettingsChanged = true;
	GLuint mUniformBlockTextureSettingsIndex = 0;
	GLuint mUniformBlockTextureSettingsUBO = 0;

	GLuint mUniformTextureUnits[MaxRenderInstances * MaxTextures];
	GLuint mUniformTextureUnitIndexes[MaxRenderInstances * MaxTextures];
	bool mUniformTextureUnitsChanged = true;

	UniformBlockMaterial mUniformBlockMaterial[MaxRenderInstances];
	bool mUniformBlockMaterialChanged = true;
	GLuint mUniformBlockMaterialIndex = 0;
	GLuint mUniformBlockMaterialUBO = 0;

	UniformBlockLight mUniformBlockLights[MaxRenderInstances * MaxLights];
	bool mUniformBlockLightsChanged = true;
	GLuint mUniformBlockLightsIndex = 0;
	GLuint mUniformBlockLightsUBO = 0;

	// TODO: Render resolution and previous-frame texture removed
	//       These may be needed later, but perhaps that should be
	//       a different shader / interface for that one

	GLuint mProgram = 0;

	std::string mVertexShaderPath;
	std::string mFragmentShaderPath;
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

