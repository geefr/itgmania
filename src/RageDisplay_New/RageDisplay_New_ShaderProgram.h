#pragma once

#include <RageTypes.h>

#include <GL/glew.h>

#include <ctype.h>
#include <RageDisplay.h>

/**
 * Wrapper around an OpenGL shader program, along with
 * the vertex and uniform interfaces used by RageDisplay_New
 *
 * A single interface is used for both sprite-based rendering (DrawQuad and friends)
 * and compiled geometry (character models, 3D noteskins).
 * This interface is slightly different in that models don't include vertex colours,
 * but it's not work writing two separate shader apis.
 */
class RageDisplay_New_ShaderProgram
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

	struct CompiledModelVertex
	{
		RageVector3 p; // position
		RageVector3 n; // normal
		// RageVColor c; // Vertex colour - Not applicable for models
		RageVector2 t; // texcoord
		RageVector2 ts; // texture matrix scale
	};

	// All uniforms are std140
	// Padding is 1, 2, 4
	// Arrays are padded
	// Struct should be padded to 4 elements or be padded by gl
	// (Paranoid approach is to use only vec4s and specify padding)
	// TODO: All flags here are ints instead of bools to get things working.
	//       Should review this, since it's fairly wasteful. A single int
	//       could communicate the render mode just as well I guess.
	struct UniformBlockMatrices
	{
		RageMatrix modelView;
		RageMatrix projection;
		RageMatrix texture;

		GLint enableAlphaTest = true;
		GLint enableLighting = false;
		GLint enableVertexColour = true;
		GLint enableTextureMatrixScale = false;

		GLfloat alphaTestThreshold = 1.0f / 256.0f;
		GLfloat pad1 = 0.0f;
		GLfloat pad2 = 0.0f;
		GLfloat pad3 = 0.0f;

		bool operator==(const UniformBlockMatrices& o) const {
			if (modelView != o.modelView) return false;
			if (projection != o.projection) return false;
			if (texture != o.texture) return false;
			if (enableAlphaTest != o.enableAlphaTest) return false;
			if (enableLighting != o.enableLighting) return false;
			if (enableVertexColour != o.enableVertexColour) return false;
			if (enableTextureMatrixScale != o.enableTextureMatrixScale) return false;
			if (alphaTestThreshold != o.alphaTestThreshold) return false;
			return true;
		}
		bool operator!=(const UniformBlockMatrices& o) const { return !operator==(o); }
	};
	struct UniformBlockTextureSettings
	{
		GLint enabled = false;
		GLint envMode = TextureMode_Modulate;

		bool operator==(const UniformBlockTextureSettings& o) const {
			if (enabled != o.enabled) return false;
			if (envMode != o.envMode) return false;
			return true;
		}
		bool operator!=(const UniformBlockTextureSettings& o) const { return !operator==(o); }
	};
	struct UniformBlockMaterial
	{
		RageVector4 emissive = { 0.0f, 0.0f, 0.0f, 1.0f };
		RageVector4 ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
		RageVector4 diffuse = { 0.8f, 0.8f, 0.8f, 1.0f };
		RageVector4 specular = { 0.0f, 0.0f, 0.0f, 1.0f };
		GLfloat shininess = 0.0f;
		GLfloat pad4 = 0.0f;
		GLfloat pad5 = 0.0f;
		GLfloat pad6 = 0.0f;

		bool operator==(const UniformBlockMaterial& o) const {
			if (emissive != o.emissive) return false;
			if (ambient != o.ambient) return false;
			if (diffuse != o.diffuse) return false;
			if (specular != o.specular) return false;
			if (shininess != o.shininess) return false;
			return true;
		}
		bool operator!=(const UniformBlockMaterial& o) const { return !operator==(o); }
	};
	struct UniformBlockLight
	{
		RageVector4 ambient;
		RageVector4 diffuse;
		RageVector4 specular;
		RageVector4 position;
		GLint enabled = false;
		GLint pad7 = 0;
		GLint pad8 = 0;
		GLint pad9 = 0;

		bool operator==(const UniformBlockLight& o) const {
			if (ambient != o.ambient) return false;
			if (diffuse != o.diffuse) return false;
			if (specular != o.specular) return false;
			if (position != o.position) return false;
			if (enabled != o.enabled) return false;
			return true;
		}
		bool operator!=(const UniformBlockLight& o) const { return !operator==(o); }
	};

	RageDisplay_New_ShaderProgram();
	RageDisplay_New_ShaderProgram(std::string vertShader, std::string fragShader);
	~RageDisplay_New_ShaderProgram();

	const UniformBlockMatrices& uniformMatrices() const { return mUniformBlockMatrices; }
	void setUniformMatrices(const UniformBlockMatrices& block);

	const UniformBlockTextureSettings& uniformTextureSettings(const uint32_t index) const { return mUniformBlockTextureSettings[index]; }
	void setUniformTextureSettings(const uint32_t index, const UniformBlockTextureSettings& block);

	const TextureUnit uniformTextureUnit(const uint32_t index) const { return static_cast<TextureUnit>(mUniformTextureUnits[index]); }
	void setUniformTextureUnit(const uint32_t index, const TextureUnit& unit);

	const UniformBlockMaterial& uniformMaterial() const { return mUniformBlockMaterial; }
	void setUniformMaterial(const UniformBlockMaterial& block);

	const UniformBlockLight& uniformLight(const uint32_t index) const { return mUniformBlockLights[index]; }
	void setUniformLight(const uint32_t index, const UniformBlockLight& block);

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
	UniformBlockMatrices mUniformBlockMatrices;
	bool mUniformBlockMatricesChanged = true;
	GLuint mUniformBlockMatricesIndex = 0;
	GLuint mUniformBlockMatricesUBO = 0;

	UniformBlockTextureSettings mUniformBlockTextureSettings[MaxTextures];
	bool mUniformBlockTextureSettingsChanged = true;
	GLuint mUniformBlockTextureSettingsIndex = 0;
	GLuint mUniformBlockTextureSettingsUBO = 0;

	GLuint mUniformTextureUnits[MaxTextures];
	GLuint mUniformTextureUnitIndexes[MaxTextures];
	bool mUniformTextureUnitsChanged = true;

	UniformBlockMaterial mUniformBlockMaterial;
	bool mUniformBlockMaterialChanged = true;
	GLuint mUniformBlockMaterialIndex = 0;
	GLuint mUniformBlockMaterialUBO = 0;

	UniformBlockLight mUniformBlockLights[MaxLights];
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
