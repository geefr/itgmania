#pragma once

#include <global.h>
#include <RageTypes.h>

#include <GL/glew.h>

namespace RageDisplay_GL4
{
	enum class ShaderName
	{
		Invalid,
		MegaShader,
		MegaShaderCompiledGeometry,
		FlipFlopFinal,

		// TODO: All of these, as shaders, or as part of MegaShader
		TextureMatrixScaling,
		Shell,
		Cel,
		DistanceField,
		Unpremultiply,
		ColourBurn,
		ColourDodge,
		VividLight,
		HardMix,
		Overlay,
		Screen,
		YUYV422,
	};

	// (RageSpriteVertex)
	struct SpriteVertex
	{
		RageVector3 p; // position
		RageVector3 n; // normal
		RageVector4 c; // vertex colour
		RageVector2 t; // texcoord
		// RageVector2 ts; // texture matrix scale - Not applicable for sprites
	};

	// (No Rage Equivalent)
	struct CompiledModelVertex
	{
		RageVector3 p; // position
		RageVector3 n; // normal
		// RageVector4 c; // certex colour - Not applicable for models
		RageVector2 t;	// texcoord
		RageVector2 ts; // texture matrix scale
	};

	// All uniforms are std140
	// TODO: Should better organise based on how often they're changed, 
	//       and remove padding where possible
	struct UniformBlockMatrices
	{
		RageMatrix modelView = {};
		RageMatrix projection = {};
		RageMatrix texture {};

		GLint enableAlphaTest = true;
		GLint enableLighting = false;
		GLint enableVertexColour = true;
		GLint enableTextureMatrixScale = false;

		GLfloat alphaTestThreshold = 1.0f / 256.0f;
		GLfloat pad1 = 0.0f;
		GLfloat pad2 = 0.0f;
		GLfloat pad3 = 0.0f;

		bool operator==(const UniformBlockMatrices &o) const
		{
			if (modelView != o.modelView)
				return false;
			if (projection != o.projection)
				return false;
			if (texture != o.texture)
				return false;
			if (enableAlphaTest != o.enableAlphaTest)
				return false;
			if (enableLighting != o.enableLighting)
				return false;
			if (enableVertexColour != o.enableVertexColour)
				return false;
			if (enableTextureMatrixScale != o.enableTextureMatrixScale)
				return false;
			if (alphaTestThreshold != o.alphaTestThreshold)
				return false;
			return true;
		}
		bool operator!=(const UniformBlockMatrices &o) const { return !operator==(o); }

		UniformBlockMatrices operator=(const UniformBlockMatrices& o)
		{
			modelView = o.modelView;
			projection = o.projection;
			texture = o.texture;
			enableAlphaTest = o.enableAlphaTest;
			enableLighting = o.enableLighting;
			enableVertexColour = o.enableVertexColour;
			enableTextureMatrixScale = o.enableTextureMatrixScale;
			alphaTestThreshold = o.alphaTestThreshold;
			pad1 = o.pad1;
			pad2 = o.pad2;
			pad3 = o.pad3;
			return *this;
		}
	};

	struct UniformBlockTextureSettings
	{
		GLint enabled = false;
		GLint envMode = TextureMode_Modulate;
		// TODO: Parameters for COMBINE / TextureMode_Glow
		GLint pad4 = 0;
		GLint pad5 = 0;

		bool operator==(const UniformBlockTextureSettings &o) const
		{
			if (enabled != o.enabled)
				return false;
			if (envMode != o.envMode)
				return false;
			return true;
		}
		bool operator!=(const UniformBlockTextureSettings &o) const { return !operator==(o); }

		UniformBlockTextureSettings operator=(const UniformBlockTextureSettings& o)
		{
			enabled = o.enabled;
			envMode = o.envMode;
			pad4 = o.pad4;
			pad5 = o.pad5;
			return *this;
		}
	};

	struct UniformBlockMaterial
	{
		RageVector4 emissive = {0.0f, 0.0f, 0.0f, 1.0f};
		RageVector4 ambient = {0.2f, 0.2f, 0.2f, 1.0f};
		RageVector4 diffuse = {0.8f, 0.8f, 0.8f, 1.0f};
		RageVector4 specular = {0.0f, 0.0f, 0.0f, 1.0f};
		GLfloat shininess = 0.0f;
		GLfloat pad6 = 0.0f;
		GLfloat pad7 = 0.0f;
		GLfloat pad8 = 0.0f;

		bool operator==(const UniformBlockMaterial &o) const
		{
			if (emissive != o.emissive)
				return false;
			if (ambient != o.ambient)
				return false;
			if (diffuse != o.diffuse)
				return false;
			if (specular != o.specular)
				return false;
			if (shininess != o.shininess)
				return false;
			return true;
		}
		bool operator!=(const UniformBlockMaterial &o) const { return !operator==(o); }

		UniformBlockMaterial operator=(const UniformBlockMaterial& o)
		{
			emissive = o.emissive;
			ambient = o.ambient;
			diffuse = o.diffuse;
			specular = o.specular;
			shininess = o.shininess;
			pad6 = o.pad6;
			pad7 = o.pad7;
			pad8 = o.pad8;
			return *this;
		}
	};

	struct UniformBlockLight
	{
		RageVector4 ambient;
		RageVector4 diffuse;
		RageVector4 specular;
		RageVector4 position;
		GLint enabled = false;
		GLint pad9 = 0;
		GLint pad10 = 0;
		GLint pad11 = 0;

		bool operator==(const UniformBlockLight &o) const
		{
			if (ambient != o.ambient)
				return false;
			if (diffuse != o.diffuse)
				return false;
			if (specular != o.specular)
				return false;
			if (position != o.position)
				return false;
			if (enabled != o.enabled)
				return false;
			return true;
		}
		bool operator!=(const UniformBlockLight &o) const { return !operator==(o); }

		UniformBlockLight operator=(const UniformBlockLight& o)
		{
			ambient = o.ambient;
			diffuse = o.diffuse;
			specular = o.specular;
			position = o.position;
			enabled = o.enabled;
			pad9 = o.pad9;
			pad10 = o.pad10;
			pad11 = o.pad11;
			return *this;
		}
	};
}
