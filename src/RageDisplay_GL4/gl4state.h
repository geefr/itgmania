#pragma once

#include <global.h>
#include <RageTypes.h>

#include <GL/glew.h>

#include "shaderprogram.h"

namespace RageDisplay_GL4
{
	class State
	{
	public:
		struct PerTextureState
		{
			bool equivalent(const PerTextureState& o) const;
			// We use the default per-texture samplers at the moment
			// so these updates must be done after glActiveTexture and glBindTexture!
			void updateGPUStateCurrentTextureUnit() const;
			void updateGPUStateCurrentTextureUnit(const PerTextureState& p) const;

	    PerTextureState& operator=(const PerTextureState& o);

			// Not actually GPU state
			bool hasMipMaps = false;

			// Strictly speaking these are per-texture-sampler
			// settings, but currently we use glTextureParameter
			// and each texture has its own built-in sampler.
			// This makes state tracking a little awkward,
			// but is inline with the legacy renderer / SM's expectations.
			GLenum wrapMode = GL_REPEAT;
			GLenum minFilter = GL_NEAREST_MIPMAP_LINEAR;
			GLenum magFilter = GL_LINEAR;
		};

		struct GlobalState
		{
			bool equivalent(const GlobalState& o) const;
			void updateGPUState() const;
			void updateGPUState(const GlobalState& p) const;

			GlobalState& operator=(const GlobalState& o);

			bool depthWriteEnabled = true;
			GLenum depthFunc = GL_LESS;
			float depthNear = 0.0f;
			float depthFar = 1.0f;
			GLenum blendEq = GL_FUNC_ADD;
			GLenum blendSourceRGB = GL_ONE;
			GLenum blendDestRGB = GL_ZERO;
			GLenum blendSourceAlpha = GL_ONE;
			GLenum blendDestAlpha = GL_ZERO;
			bool cullEnabled = false;
			GLenum cullFace = GL_BACK;
			float lineWidth = 1.0f;
			float pointSize = 1.0f;
		};

		State() = default;
		~State() = default;
		State(const State&) = default;
		State(State&&) = default;

		State& operator=(const State& o);

		/// Check for equivalence with another state
		/// Note that this is a fuzzy-equivalence
		/// e.g. If a texture unit is disabled, we don't care
		///      which texture is bound to it.
		bool equivalent(const State& o) const;

		/// Update OpenGL with the current state
		/// A previous state must be provided for change-only updates
		void updateGPUState();
		void updateGPUState(const State& p);

	  /// Helper functions
		void bindTexture(GLuint unit, GLuint tex);
		GLuint boundTexture(GLuint unit) const;
		bool isTextureUnitBound(GLuint unit) const;
		void textureWrap(GLuint unit, GLenum mode);
		void textureFilter(GLuint unit, GLenum minFilter, GLenum magFilter);

		void addTexture(GLuint tex, bool hasMipMaps);
		bool textureHasMipMaps(GLuint tex) const;
		void removeTexture(GLuint tex);

		GlobalState globalState;
		// texture ID, state
		std::map<GLuint, PerTextureState> textureState;
		// texture unit, texture ID
		std::map<GLuint, GLuint> boundTextures;

		std::shared_ptr<ShaderProgram> shaderProgram;
		UniformBlockMatrices uniformBlockMatrices;
		// texture unit, uniforms
		UniformBlockTextureSettings uniformBlockTextureSettings[ShaderProgram::MaxTextures];
		UniformBlockMaterial uniformBlockMaterial;
		// texture unit index (0-based)
		UniformBlockLight uniformBlockLights[ShaderProgram::MaxLights];
	};
}
