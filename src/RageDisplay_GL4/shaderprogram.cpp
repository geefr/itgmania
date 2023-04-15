
#include <global.h>

#include "shaderprogram.h"

#include <RageLog.h>
#include <RageFile.h>

namespace RageDisplay_GL4
{

void ShaderProgram::configureVertexAttributesForSpriteRender()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, p)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, n)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, c)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex), reinterpret_cast<const void*>(offsetof(SpriteVertex, t)));
	glEnableVertexAttribArray(3);
	// 4 reserved - texture scaling
}

void ShaderProgram::configureVertexAttributesForCompiledRender()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(CompiledModelVertex, p)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(CompiledModelVertex, n)));
	glEnableVertexAttribArray(1);
	// 2 reserved - vertex colour
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(CompiledModelVertex, t)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(CompiledModelVertex, ts)));
	glEnableVertexAttribArray(4);
}

GLuint ShaderProgram::LoadShaderProgram(std::string vert, std::string frag, bool assertOnError)
{
	auto vertShader = ShaderProgram::LoadShader(GL_VERTEX_SHADER, vert, assertOnError);
	auto fragShader = ShaderProgram::LoadShader(GL_FRAGMENT_SHADER, frag, assertOnError);
	if (vertShader == 0 || fragShader == 0)
	{
		return 0;
	}

	auto shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertShader);
	glAttachShader(shaderProgram, fragShader);
	glLinkProgram(shaderProgram);

	GLint success = 0;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

	if (success)
	{
		return shaderProgram;
	}
	else
	{
		GLint logLength = 0;
		glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(logLength, '\0');
		glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
		if (assertOnError)
		{
			ASSERT_M(success, (std::string("Failed to link shader program: ") + log).c_str());
		}
		else
		{
			LOG->Info("Failed to link shader program: %s", log.c_str());
		}
	}
	return 0;
}

GLuint ShaderProgram::LoadShader(GLenum type, std::string source, bool assertOnError)
{
	RString buf;
	{
		RageFile file;
		if (!file.Open(source))
		{
			LOG->Warn("Error compiling shader %s: %s", source.c_str(), file.GetError().c_str());
			return 0;
		}

		if (file.Read(buf, file.GetFileSize()) == -1)
		{
			LOG->Warn("Error compiling shader %s: %s", source.c_str(), file.GetError().c_str());
			return 0;
		}
	}

	auto shader = glCreateShader(type);
	const char* cStr = buf.c_str();
	glShaderSource(shader, 1, &cStr, nullptr);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (success)
	{
		return shader;
	}
	else
	{
		// TODO: Get error log
		if (assertOnError)
		{
			ASSERT_M(success, (std::string("Failed to compile shader ") + source).c_str());
		}
		else
		{
			LOG->Info("Failed to compile shader %s", source.c_str());
		}
	}
	return 0;
}

ShaderProgram::ShaderProgram() {}

ShaderProgram::ShaderProgram(std::string vertShader, std::string fragShader)
	: mVertexShaderPath(vertShader)
	, mFragmentShaderPath(fragShader)
{}

ShaderProgram::~ShaderProgram()
{
	invalidate();
}

bool ShaderProgram::init()
{
	invalidate();

	mProgram = ShaderProgram::LoadShaderProgram(
		mVertexShaderPath,
		mFragmentShaderPath,
		false
	);
	if (mProgram == 0)
	{
		return false;
	}

	initUniforms();

	bind();

	updateUniforms();

	return true;
}

void ShaderProgram::initUniforms()
{
  // Note: The name of blocks in the shader is the word after 'uniform' i.e. The name of the uniform block.
  //       The name after the block definition in the shader is just a local alias.
  // Hours wasted: 2
	mUniformBlockMatricesIndex = glGetUniformBlockIndex(mProgram, "MatricesBlock");
	mUniformBlockTextureSettingsIndex = glGetUniformBlockIndex(mProgram, "TextureSettingsBlock");
	for (auto i = 0; i < MaxTextures; ++i)
	{
		mUniformTextureUnitIndexes[i] = glGetUniformLocation(mProgram,
			(std::string("Texture[") + std::to_string(i) + std::string("]")).c_str());
	}
	mUniformBlockMaterialIndex = glGetUniformBlockIndex(mProgram, "MaterialBlock");
	mUniformBlockLightsIndex = glGetUniformBlockIndex(mProgram, "LightsBlock");

	if (mUniformBlockMatricesIndex != GL_INVALID_INDEX)
	{
		glGenBuffers(1, &mUniformBlockMatricesUBO);
	}

	if (mUniformBlockTextureSettingsIndex != GL_INVALID_INDEX)
	{
		glGenBuffers(1, &mUniformBlockTextureSettingsUBO);
	}

	if (mUniformBlockMaterialIndex != GL_INVALID_INDEX)
	{
		glGenBuffers(1, &mUniformBlockMaterialUBO);
	}

	if (mUniformBlockLightsIndex != GL_INVALID_INDEX)
	{
		glGenBuffers(1, &mUniformBlockLightsUBO);
	}
}

void ShaderProgram::invalidate()
{
	mUniformBlockMatrices = {};
	for (auto i = 0; i < MaxTextures; ++i) mUniformBlockTextureSettings[i] = {};
	for (auto i = 0; i < MaxTextures; ++i) mUniformTextureUnits[i] = 0;
	mUniformBlockMaterial = {};
	for (auto i = 0; i < MaxLights; ++i) mUniformBlockLights[i] = {};

	mUniformBlockMatricesChanged = true;
	mUniformBlockTextureSettingsChanged = true;
	mUniformBlockMaterialChanged = true;
	mUniformBlockLightsChanged = true;

	mUniformBlockMatricesIndex = 0;
	mUniformBlockTextureSettingsIndex = 0;
	for (auto i = 0; i < MaxTextures; ++i) mUniformTextureUnitIndexes[i] = 0;
	mUniformBlockMaterialIndex = 0;
	mUniformBlockLightsIndex = 0;

	if (mUniformBlockMatricesUBO)
	{
		glDeleteBuffers(1, &mUniformBlockMatricesUBO);
		mUniformBlockMatricesUBO = 0;
	}
	if (mUniformBlockTextureSettingsUBO)
	{
		glDeleteBuffers(1, &mUniformBlockTextureSettingsUBO);
		mUniformBlockTextureSettingsUBO = 0;
	}
	if (mUniformBlockMaterialUBO)
	{
		glDeleteBuffers(1, &mUniformBlockMaterialUBO);
		mUniformBlockMaterialUBO = 0;
	}
	if (mUniformBlockLightsUBO)
	{
		glDeleteBuffers(1, &mUniformBlockLightsUBO);
		mUniformBlockLightsUBO = 0;
	}

	if (mProgram)
	{
		glDeleteProgram(mProgram);
		mProgram = 0;
	}
}

void ShaderProgram::bind()
{
	glUseProgram(mProgram);

	if( mUniformBlockMatricesIndex != GL_INVALID_INDEX )
	{
		// TODO: Re-binding these here is needed, since we're re-using the indices across shader instances.
		//       The uniforms should be split out to a separate class - They belong to the renderer, not
		//       each specific shader index.
		//       This would also go well with preprocessing the shader variants - Allowing the uniform blocks
		//       to only contain matrices / changing data, with rendering parameters preconfigured in each
		//       shader instance.
		//       For now, and as this is already way faster than using individual uniforms in the default block,
		//       just re-use the same binding indexes for all programs. There's larger things to solve first.
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockMatricesUBO);
		// Requires allocated buffer, is context state, not program (can share uniform buffers across programs)
		glBindBufferBase(GL_UNIFORM_BUFFER, 0, mUniformBlockMatricesUBO);
		// You shouldn't be doing this every time - It's program state (to point to the ubo binding table)
		glUniformBlockBinding(mProgram, mUniformBlockMatricesIndex, 0);
	}

	if( mUniformBlockTextureSettingsIndex != GL_INVALID_INDEX )
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockTextureSettingsUBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, 1, mUniformBlockTextureSettingsUBO);
		glUniformBlockBinding(mProgram, mUniformBlockTextureSettingsIndex, 1);
	}

	if (mUniformBlockMaterialIndex != GL_INVALID_INDEX)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockMaterialUBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, mUniformBlockMaterialUBO);
		glUniformBlockBinding(mProgram, mUniformBlockMaterialIndex, 2);
	}

	if (mUniformBlockTextureSettingsIndex != GL_INVALID_INDEX)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockLightsUBO);
		glBindBufferBase(GL_UNIFORM_BUFFER, 3, mUniformBlockLightsUBO);
		glUniformBlockBinding(mProgram, mUniformBlockLightsIndex, 3);
	}
}

void ShaderProgram::updateUniforms()
{
	if (mUniformBlockMatricesChanged && mUniformBlockMatricesUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockMatricesUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockMatrices), &mUniformBlockMatrices, GL_STREAM_DRAW);
		mUniformBlockMatricesChanged = false;
	}

	if (mUniformBlockTextureSettingsChanged && mUniformBlockTextureSettingsUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockTextureSettingsUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockTextureSettings) * MaxTextures, &mUniformBlockTextureSettings, GL_STREAM_DRAW);
		mUniformBlockTextureSettingsChanged = false;
	}

	if (mUniformBlockMaterialChanged && mUniformBlockMaterialUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockMaterialUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockMaterial), &mUniformBlockMaterial, GL_STREAM_DRAW);
		mUniformBlockMaterialChanged = false;
	}

	if (mUniformBlockLightsChanged && mUniformBlockLightsUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockLightsUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockLight) * MaxLights, &mUniformBlockLights, GL_STREAM_DRAW);
		mUniformBlockLightsChanged = false;
	}

	if (mUniformTextureUnitsChanged)
	{
		for (auto i = 0; i < MaxTextures; ++i)
		{
			if (mUniformTextureUnitIndexes[i])
			{
				if(mUniformBlockTextureSettings[i].enabled)
				{
					glUniform1i(mUniformTextureUnitIndexes[i], mUniformTextureUnits[i]);
				}
			}
		}
		mUniformTextureUnitsChanged = false;
	}
}

bool ShaderProgram::setUniformMatrices(const UniformBlockMatrices& block)
{
	if (mUniformBlockMatrices != block)
	{
		mUniformBlockMatrices = block;
		mUniformBlockMatricesChanged = true;
		return true;
	}
	return false;
}

bool ShaderProgram::setUniformTextureSettings(const uint32_t index, const UniformBlockTextureSettings& block)
{
	if (mUniformBlockTextureSettings[index] != block)
	{
		mUniformBlockTextureSettings[index] = block;
		mUniformBlockTextureSettingsChanged = true;
		return true;
	}
	return false;
}

bool ShaderProgram::setUniformTextureUnit(const uint32_t index, const TextureUnit& unit)
{
	if (mUniformTextureUnits[index] != static_cast<GLuint>(unit))
	{
		mUniformTextureUnits[index] = static_cast<GLuint>(unit);
		mUniformTextureUnitsChanged = true;
		return true;
	}
	return false;
}

bool ShaderProgram::setUniformMaterial(const UniformBlockMaterial& block)
{
	if (mUniformBlockMaterial != block)
	{
		mUniformBlockMaterial = block;
		mUniformBlockMaterialChanged = true;
		return true;
	}
	return false;
}

bool ShaderProgram::setUniformLight(const uint32_t index, const UniformBlockLight& block)
{
	if (mUniformBlockLights[index] != block)
	{
		mUniformBlockLights[index] = block;
		mUniformBlockLightsChanged = true;
		return true;
	}
	return false;
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

