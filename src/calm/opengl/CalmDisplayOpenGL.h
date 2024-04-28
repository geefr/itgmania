
#pragma once

#include "calm/CalmDisplay.h"
#include "calm/opengl/CalmDrawableFactoryOpenGL.h"
#include "calm/opengl/CalmShaderProgramOpenGL.h"

#include <iostream>
#include <vector>
#include <list>
#include <map>
#include <set>

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
		//
		// TODO: These are hardcoded for now, but need rework
		// - More than just sprites are rendered, first part becomes overall type of draw
		// - Effect mode and texture mode are exclusive
		// - There'll be a lot of combinations
		// -> Add a shader preprocessor with the basics
		//    - #include is needed for renderdoc to work
		//    - #if / #define are needed to call into texture and effect mode functions

		// The overall base shader
		enum class ShaderBase {
			Sprite,
			MultiTexture,
		};
		// Enabled features
		enum class ShaderFeature {
			// uniform sampler2D textureN
			// uniform bool textureNEnabled
			// ##TEXTURE_N## -> textureN
			TextureUnit0,
			TextureUnit1,
			TextureUnit2,
			TextureUnit3,
		};

		// Unique ID for a shader combination
		struct ShaderParameters
		{
			ShaderBase base = ShaderBase::Sprite;
			std::set<ShaderFeature> features = {};
			// ##TEXTUREMODE_N## -> textureMode_textureN(vec4 c, vec4 uv)
			TextureMode textureMode0 = TextureMode::TextureMode_Modulate;
			TextureMode textureMode1 = TextureMode::TextureMode_Modulate;
			TextureMode textureMode2 = TextureMode::TextureMode_Modulate;
			TextureMode textureMode3 = TextureMode::TextureMode_Modulate;
			// ##EFFECTMODE## -> effectMode(vec4 c, vec4 uv)
			EffectMode effectMode = EffectMode::EffectMode_Normal;

			inline bool operator==(const ShaderParameters& r) { 
				return base == r.base &&
				       features == r.features &&
					   textureMode0 == r.textureMode0 &&
					   textureMode1 == r.textureMode1 &&
					   textureMode2 == r.textureMode2 &&
					   textureMode3 == r.textureMode3 &&
					   effectMode == r.effectMode;
			}
		};

	    DisplayOpenGL();
		virtual ~DisplayOpenGL();

		DrawableFactory& drawables() override { return mDrawables; }

		void setRenderState(RenderState state) override;

		void contextLost() override;
		void resolutionChanged(unsigned int w, unsigned int h) override;
		std::string getDebugInformationString() override;
		void init(InitParameters p) override;
		void doDraw(std::vector<std::shared_ptr<Drawable>>&& d) override;
		void doSync() override;
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
		void setTextureWrapping( uintptr_t texture, bool wrap ) override;
		void setTextureFiltering( uintptr_t texture, bool filter ) override;

		std::shared_ptr<ShaderProgram> getShader(ShaderParameters params, bool allowCompile = true);
		void clearShaders() { mShaders.clear(); }

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

		bool mTrilinearFilteringEnabled = false;

		std::vector<std::pair<ShaderParameters, std::shared_ptr<ShaderProgram>>> mShaders;

		// GL Fences to allow a desired frames-in-flight, but
		// without a large stall from glFinish()
		const bool frameSyncUsingFences = true;
		const uint32_t frameSyncDesiredFramesInFlight = 2;
		std::list<GLsync> frameSyncFences;
	};
}
