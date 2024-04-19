
#pragma once

#include "calm/CalmDisplay.h"
#include "calm/opengl/CalmDrawableFactoryOpenGL.h"
#include "calm/opengl/CalmShaderProgramOpenGL.h"

#include <iostream>
#include <vector>
#include <map>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

namespace calm {
	class DisplayOpenGL final : public Display
	{
	public:

		// Drawabletype_shadermode0_shadermode1_shadermode2_shadermode3
		// Shaders are minimised, based on num textures used by each drawable
		// Initialised by calm::RageAdapter::loadOpenGLShaders
		// (since we need access to RageFile)
		enum class ShaderName {
			Sprite_Modulate0,
			Sprite_Glow0,
		};

	    DisplayOpenGL();
		virtual ~DisplayOpenGL();

		DrawableFactory& drawables() override { return mDrawables; }

		void contextLost() override;
		void resolutionChanged(unsigned int w, unsigned int h) override;
		std::string getDebugInformationString() override;
		void init() override;
		void doDraw(std::vector<std::shared_ptr<Drawable>>&& d) override;
		int maxTextureSize() const override { return mMaxTextureSize; }

		std::uintptr_t createTexture(
			TextureFormat format,
			uint8_t* pixels,
			uint32_t w, uint32_t h,
			uint32_t pitch, uint32_t bytesPerPixel,
			bool generateMipMaps) override;
		// void UpdateTexture(
		// 	std::uintptr_t iTexHandle,
		// 	RageSurface* img,
		// 	int xoffset, int yoffset, int width, int height);
		void deleteTexture( std::uintptr_t iTexHandle ) override;


		void clearShaders() { mShaders.clear(); }
		const std::map<ShaderName, std::shared_ptr<ShaderProgram>>& shaders() const { return mShaders; }
		void setShader(ShaderName name, std::shared_ptr<ShaderProgram> shader) { mShaders[name] = shader; }

		

	private:
		DrawableFactoryOpenGL mDrawables;

		inline static std::string getString(GLenum name) {
			if( auto c = glGetString(name) ) return std::string(reinterpret_cast<const char*>(c));
			return {};
		}
		inline static GLint getInt(GLenum name) {
			GLint v;
			glGetIntegerv(name, &v);
			return v;
		}
		inline static GLfloat getFloat(GLenum name) {
			GLfloat v;
			glGetFloatv(name, &v);
			return v;
		}

		// Global parameters - For debug string and similar checks
		std::string mGLVendor;
		std::string mGLRenderer;
		std::string mGLVersion;
		GLint mGLProfileMask;
		std::string mGLUVersion;
		GLint mNumTextureUnits;
		GLint mNumTextureUnitsCombined;
		GLint mTextureUnitForTexUploads;
		GLint mMaxTextureSize;
		GLuint mVAO;

		GLuint mWidth = 0;
		GLuint mHeight = 0;

		std::map<ShaderName, std::shared_ptr<ShaderProgram>> mShaders;
	};
}
