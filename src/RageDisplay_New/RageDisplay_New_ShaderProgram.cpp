
#include <global.h>

#include "RageDisplay_New_ShaderProgram.h"

#include <RageLog.h>
#include <RageFile.h>

void RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, p)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, n)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, c)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, t)));
	glEnableVertexAttribArray(3);
	// 4 reserved - texture scaling
}

void RageDisplay_New_ShaderProgram::configureVertexAttributesForCompiledRender()
{
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, p)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, n)));
	glEnableVertexAttribArray(1);
	// 2 reserved - vertex colour
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(CompiledModelVertex, t)));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(CompiledModelVertex), reinterpret_cast<const void*>(offsetof(CompiledModelVertex, ts)));
	glEnableVertexAttribArray(4);
}

GLuint RageDisplay_New_ShaderProgram::LoadShaderProgram(std::string vert, std::string frag, bool assertOnError)
{
	auto vertShader = RageDisplay_New_ShaderProgram::LoadShader(GL_VERTEX_SHADER, vert, assertOnError);
	auto fragShader = RageDisplay_New_ShaderProgram::LoadShader(GL_FRAGMENT_SHADER, frag, assertOnError);
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

GLuint RageDisplay_New_ShaderProgram::LoadShader(GLenum type, std::string source, bool assertOnError)
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

RageDisplay_New_ShaderProgram::RageDisplay_New_ShaderProgram() {}

RageDisplay_New_ShaderProgram::RageDisplay_New_ShaderProgram(std::string vertShader, std::string fragShader)
	: mVertexShaderPath(vertShader)
	, mFragmentShaderPath(fragShader)
{}

RageDisplay_New_ShaderProgram::~RageDisplay_New_ShaderProgram()
{
	invalidate();
}

bool RageDisplay_New_ShaderProgram::init()
{
	invalidate();

	mProgram = RageDisplay_New_ShaderProgram::LoadShaderProgram(
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

void RageDisplay_New_ShaderProgram::initUniforms()
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

void RageDisplay_New_ShaderProgram::invalidate()
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

void RageDisplay_New_ShaderProgram::bind()
{
	glUseProgram(mProgram);

	if( mUniformBlockMatricesIndex != GL_INVALID_INDEX )
	{
		// TODO: Re-binding these here is needed, since we may have multiple programs with different uniform values
		//       The uniform buffers should be in a separate class from the shader, to minimise this.
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

void RageDisplay_New_ShaderProgram::updateUniforms()
{
	if (mUniformBlockMatricesChanged && mUniformBlockMatricesUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockMatricesUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockMatrices), &mUniformBlockMatrices, GL_DYNAMIC_DRAW);
		mUniformBlockMatricesChanged = false;
	}

	if (mUniformBlockTextureSettingsChanged && mUniformBlockTextureSettingsUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockTextureSettingsUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockTextureSettings) * MaxTextures, &mUniformBlockTextureSettings, GL_DYNAMIC_DRAW);
		mUniformBlockTextureSettingsChanged = false;
	}

	if (mUniformBlockMaterialChanged && mUniformBlockMaterialUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockMaterialUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockMaterial), &mUniformBlockMaterial, GL_DYNAMIC_DRAW);
		mUniformBlockMaterialChanged = false;
	}

	if (mUniformBlockLightsChanged && mUniformBlockLightsUBO)
	{
		glBindBuffer(GL_UNIFORM_BUFFER, mUniformBlockLightsUBO);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(UniformBlockLight) * MaxLights, &mUniformBlockLights, GL_DYNAMIC_DRAW);
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

void RageDisplay_New_ShaderProgram::setUniformMatrices(const UniformBlockMatrices& block)
{
	if (mUniformBlockMatrices != block)
	{
		mUniformBlockMatrices = block;
		mUniformBlockMatricesChanged = true;
	}
}

void RageDisplay_New_ShaderProgram::setUniformTextureSettings(const uint32_t index, const UniformBlockTextureSettings& block)
{
	if (mUniformBlockTextureSettings[index] != block)
	{
		mUniformBlockTextureSettings[index] = block;
		mUniformBlockTextureSettingsChanged = true;
	}
}

void RageDisplay_New_ShaderProgram::setUniformTextureUnit(const uint32_t index, const TextureUnit& unit)
{
	if (mUniformTextureUnits[index] != static_cast<GLuint>(unit))
	{
		mUniformTextureUnits[index] = static_cast<GLuint>(unit);
		mUniformTextureUnitsChanged = true;
	}
}

void RageDisplay_New_ShaderProgram::setUniformMaterial(const UniformBlockMaterial& block)
{
	if (mUniformBlockMaterial != block)
	{
		mUniformBlockMaterial = block;
		mUniformBlockMaterialChanged = true;
	}
}

void RageDisplay_New_ShaderProgram::setUniformLight(const uint32_t index, const UniformBlockLight& block)
{
	if (mUniformBlockLights[index] != block)
	{
		mUniformBlockLights[index] = block;
		mUniformBlockLightsChanged = true;
	}
}
