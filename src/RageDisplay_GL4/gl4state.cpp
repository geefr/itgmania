
#include "gl4state.h"

namespace RageDisplay_GL4
{
	bool State::PerTextureState::equivalent(const PerTextureState& o) const
	{
		if (hasMipMaps != o.hasMipMaps) return false;
		if (wrapMode != o.wrapMode) return false;
		if (minFilter != o.minFilter) return false;
		if (magFilter != o.magFilter) return false;
		return true;
	}

	void State::PerTextureState::updateGPUStateCurrentTextureUnit() const
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	}

	void State::PerTextureState::updateGPUStateCurrentTextureUnit(const PerTextureState& p) const
	{
		if (wrapMode != p.wrapMode)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
		}
		if (minFilter != p.minFilter)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
		}
		if (magFilter != p.magFilter)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
		}
	}

	State::PerTextureState& State::PerTextureState::operator=(const State::PerTextureState& o)
	{
		hasMipMaps = o.hasMipMaps;
		wrapMode = o.wrapMode;
		minFilter = o.minFilter;
		magFilter = o.magFilter;
		return *this;
	}

	bool State::GlobalState::equivalent(const State::GlobalState& o) const
	{
		if (depthWriteEnabled != o.depthWriteEnabled) return false;
		if (depthFunc != o.depthFunc) return false;
		if (depthNear != o.depthNear) return false;
		if (depthFar != o.depthFar) return false;
		if (blendEq != o.blendEq) return false;
		if (blendSourceRGB != o.blendSourceRGB) return false;
		if (blendDestRGB != o.blendDestRGB) return false;
		if (blendSourceAlpha != o.blendSourceAlpha) return false;
		if (blendDestAlpha != o.blendDestAlpha) return false;
		if (cullEnabled != o.cullEnabled) return false;
		if (cullFace != o.cullFace) return false;
		if (lineWidth != o.lineWidth) return false;
		if (lineSmoothEnabled != o.lineSmoothEnabled) return false;
		if (pointSize != o.pointSize) return false;
		// if (pointSmoothEnabled != o.pointSmoothEnabled) return false;
		return true;
	}

	void State::GlobalState::updateGPUState() const
	{
		if (depthWriteEnabled) glDepthMask(GL_TRUE);
		else glDepthMask(GL_FALSE);
		glDepthFunc(depthFunc);
		glDepthRange(depthNear, depthFar);
		glBlendEquation(blendEq);
		glBlendFuncSeparate(blendSourceRGB, blendDestRGB, blendSourceAlpha, blendDestAlpha);
		if (cullEnabled) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);
		glCullFace(cullFace);
		glLineWidth(lineWidth);
		if (lineSmoothEnabled) glEnable(GL_LINE_SMOOTH);
		else glDisable(GL_LINE_SMOOTH);
		glPointSize(pointSize);
		// if (pointSmoothEnabled) glEnable(GL_POINT_SMOOTH);
		// else glDisable(GL_POINT_SMOOTH);
		glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);
		glViewport(
			static_cast<GLint>(viewPort.x),
			static_cast<GLint>(viewPort.y),
			static_cast<GLint>(viewPort.z),
			static_cast<GLint>(viewPort.w)
		);
	}

	void State::GlobalState::updateGPUState(const GlobalState& p) const
	{
		if (depthWriteEnabled != p.depthWriteEnabled)
		{
			if (depthWriteEnabled) glDepthMask(GL_TRUE);
			else glDepthMask(GL_FALSE);
		}
		if (depthFunc != p.depthFunc)
		{
			glDepthFunc(depthFunc);
		}
		if (depthNear != p.depthNear || depthFar != p.depthFar)
		{
			glDepthRange(depthNear, depthFar);
		}
		if (blendEq != p.blendEq)
		{
			glBlendEquation(blendEq);
		}
		if (blendSourceRGB != p.blendSourceRGB ||
			blendDestRGB != p.blendDestRGB ||
			blendSourceAlpha != p.blendSourceAlpha ||
			blendDestAlpha != p.blendDestAlpha)
		{
			glBlendFuncSeparate(blendSourceRGB, blendDestRGB, blendSourceAlpha, blendDestAlpha);
		}
		if (cullEnabled != p.cullEnabled)
		{
			if (cullEnabled) glEnable(GL_CULL_FACE);
			else glDisable(GL_CULL_FACE);
		}
		if (cullFace != p.cullFace)
		{
			glCullFace(cullFace);
		}
		if (lineWidth != p.lineWidth)
		{
			glLineWidth(lineWidth);
		}
		if (lineSmoothEnabled != p.lineSmoothEnabled)
		{
			if(lineSmoothEnabled) glEnable(GL_LINE_SMOOTH);
			else glDisable(GL_LINE_SMOOTH);
		}
		if (pointSize != p.pointSize)
		{
			glPointSize(pointSize);
		}
		// if (pointSmoothEnabled != p.pointSmoothEnabled)
		// {
		// 	if(pointSmoothEnabled) glEnable(GL_POINT_SMOOTH);
		// 	else glDisable(GL_POINT_SMOOTH);
		// }
		if (clearColour != p.clearColour)
		{
			glClearColor(clearColour.x, clearColour.y, clearColour.z, clearColour.w);
		}
		if (viewPort != p.viewPort)
		{
			glViewport(
				static_cast<GLint>(viewPort.x),
				static_cast<GLint>(viewPort.y),
				static_cast<GLint>(viewPort.z),
				static_cast<GLint>(viewPort.w)
			);
		}
	}

	State::GlobalState& State::GlobalState::operator=(const State::GlobalState& o)
	{
		depthWriteEnabled = o.depthWriteEnabled;
		depthFunc = o.depthFunc;
		depthNear = o.depthNear;
		depthFar = o.depthFar;
		blendEq = o.blendEq;
		blendSourceRGB = o.blendSourceRGB;
		blendDestRGB = o.blendDestRGB;
		blendSourceAlpha = o.blendSourceAlpha;
		blendDestAlpha = o.blendDestAlpha;
		cullEnabled = o.cullEnabled;
		cullFace = o.cullFace;
		lineWidth = o.lineWidth;
		lineSmoothEnabled = o.lineSmoothEnabled;
		pointSize = o.pointSize;
		// pointSmoothEnabled = o.pointSmoothEnabled;
		clearColour = o.clearColour;
		viewPort = o.viewPort;
		return *this;
	}

	State& State::operator=(const State& o)
	{
		globalState = o.globalState;
		textureState = o.textureState;
		boundTextures = o.boundTextures;
		shaderProgram = o.shaderProgram;
		uniformBlockMatrices = o.uniformBlockMatrices;
		for (auto i = 0; i < ShaderProgram::MaxTextures; ++i)
		{
			uniformBlockTextureSettings[i] = o.uniformBlockTextureSettings[i];
		}
		uniformBlockMaterial = o.uniformBlockMaterial;
		for (auto i = 0; i < ShaderProgram::MaxLights; ++i)
		{
			uniformBlockLights[i] = o.uniformBlockLights[i];
		}
		return *this;
	}

	bool State::equivalent(const State& o) const
	{
		if (!globalState.equivalent(o.globalState)) return false;

		if (shaderProgram != o.shaderProgram) return false;

		// For texture comparisons, we care about the currently-bound set
		// only, and only the texture units which can be used by ShaderProgram.
		// The states are only different if:
		// * Different textures are bound
		// * A currently-bound texture has been modified
		//
		// If settings for a currently-unbound texture have been modified we still
		// have those stored for the next update, but don't need to update these
		// at all until the textures have been bound for a draw.
		// TODO: This should scale fine for instanced rendering, but likely corner cases
		for (auto i = 0; i < ShaderProgram::MaxTextures; ++i)
		{
			GLuint texUnit = GL_TEXTURE0 + i;
			// Bound to anything?
			if (isTextureUnitBound(texUnit) != o.isTextureUnitBound(texUnit)) return false;
			// Different textures?
			auto textureID = boundTexture(texUnit);
			if (textureID != o.boundTexture(texUnit)) return false;
			// Texture sampler settings
			auto prevTextureState = o.textureState.find(textureID);
			if (prevTextureState == o.textureState.end()) return false;
			if (!textureState.at(textureID).equivalent(prevTextureState->second)) return false;
			// Texture uniform settings - Blending modes
			if (uniformBlockTextureSettings[i] != o.uniformBlockTextureSettings[i]) return false;
		}

		// TODO: This one will effectively prevent all batching of calls
		//       and at least the modelview matrix should be instanced
		if (uniformBlockMatrices != o.uniformBlockMatrices) return false;

		if (uniformBlockMaterial != o.uniformBlockMaterial) return false;

		if (uniformBlockMatrices.enableLighting)
		{
			if (uniformBlockLights != o.uniformBlockLights) return false;
		}
		return true;
	}

	void State::updateGPUState()
	{
		globalState.updateGPUState();
		for (auto i = 0; i < ShaderProgram::MaxTextures; ++i)
		{
			if (uniformBlockTextureSettings[i].enabled)
			{
				GLuint texUnit = GL_TEXTURE0 + i;
				auto& tex = boundTextures[i];
				glActiveTexture(texUnit);
				glBindTexture(GL_TEXTURE_2D, tex);
				textureState[tex].updateGPUStateCurrentTextureUnit();
			}
		}
		if (shaderProgram)
		{
			shaderProgram->bind();
			shaderProgram->setUniformMatrices(uniformBlockMatrices);
			for (auto i = 0; i < ShaderProgram::MaxTextures; ++i)
			{
				shaderProgram->setUniformTextureSettings(i, uniformBlockTextureSettings[i]);
			}
			shaderProgram->setUniformMaterial(uniformBlockMaterial);
			for (auto i = 0; i < ShaderProgram::MaxLights; ++i)
			{
				shaderProgram->setUniformLight(i, uniformBlockLights[i]);
			}
			shaderProgram->updateUniforms();
		}
	}
	void State::updateGPUState(const State& p)
	{
		globalState.updateGPUState(p.globalState);

		// The states are only different if:
		// * Different textures are bound
		// * A currently-bound texture has been modified
		for (auto i = 0; i < ShaderProgram::MaxTextures; ++i)
		{
			GLuint texUnit = GL_TEXTURE0 + i;
			bool texUnitActive = false;

			if (!isTextureUnitBound(texUnit)) continue;

			auto textureID = boundTexture(texUnit);
			auto prevTextureID = p.boundTexture(texUnit);

			glActiveTexture(texUnit);
			if (textureID != prevTextureID)
			{
				prevTextureID = textureID;
				glBindTexture(GL_TEXTURE_2D, textureID);
			}

			auto& texState = textureState[textureID];
			auto prevTexState = p.textureState.find(textureID);
			if (prevTexState == p.textureState.end())
			{
				texState.updateGPUStateCurrentTextureUnit();
			}
			else if (texState.equivalent(prevTexState->second))
			{
				texState.updateGPUStateCurrentTextureUnit(prevTexState->second);
			}
		}

		// TODO: Uniform state is a little wonky at the moment, RageDisplay_GL4
		//       will probably be setting all uniforms for all shaders every draw..
		//       Issue here will now be that the shader is also state-tracking the
		//       UBOs, but as we've swapped shaders it's desynced somehow?
		if (shaderProgram)
		{
			if (shaderProgram != p.shaderProgram)
			{
				shaderProgram->bind();
			}
			shaderProgram->setUniformMatrices(uniformBlockMatrices);
			for (auto i = 0; i < ShaderProgram::MaxTextures; ++i)
			{
				shaderProgram->setUniformTextureSettings(i, uniformBlockTextureSettings[i]);
			}
			shaderProgram->setUniformMaterial(uniformBlockMaterial);
			for (auto i = 0; i < ShaderProgram::MaxLights; ++i)
			{
				shaderProgram->setUniformLight(i, uniformBlockLights[i]);
			}
			shaderProgram->updateUniforms();
		}
	}

	void State::bindTexture(GLuint unit, GLuint tex)
	{
		boundTextures[unit] = tex;
		uniformBlockTextureSettings[unit - GL_TEXTURE0].enabled = tex != 0;
	}
	GLuint State::boundTexture(GLuint unit) const
	{
		auto texUnitIndex = unit - GL_TEXTURE0;
		if (texUnitIndex < ShaderProgram::MaxTextures)
		{
			if (!uniformBlockTextureSettings[unit - GL_TEXTURE0].enabled) return 0;
		}
		auto it = boundTextures.find(unit);
		if (it == boundTextures.end()) return 0;
		return it->second;
	}
	bool State::isTextureUnitBound(GLuint unit) const
	{
		return uniformBlockTextureSettings[unit - GL_TEXTURE0].enabled;
	}
	void State::textureWrap(GLuint unit, GLenum mode)
	{
		auto textureID = boundTexture(unit);
		if (textureID == 0) return;
		textureState[textureID].wrapMode = mode;
	}
	void State::textureFilter(GLuint unit, GLenum minFilter, GLenum magFilter)
	{
		auto textureID = boundTexture(unit);
		if (textureID == 0) return;
		textureState[textureID].magFilter = magFilter;
		textureState[textureID].minFilter = minFilter;
	}

	void State::addTexture(GLuint tex, bool hasMipMaps)
	{
		textureState[tex] = {};
		textureState[tex].hasMipMaps = hasMipMaps;
	}
	bool State::textureHasMipMaps(GLuint tex) const
	{
		auto it = textureState.find(tex);
		if (it == textureState.end()) return false;
		return it->second.hasMipMaps;
	}
	void State::removeTexture(GLuint tex)
	{
		textureState.erase(tex);
	}
}
