
#include "CalmDisplayOpenGL.h"

namespace calm {
	struct GLFormatForTextureFormat {
		GLenum internalfmt;
		GLenum format;
		GLenum type;
	};
	// Not const, because callers aren't const..
	static std::map<Display::TextureFormat, GLFormatForTextureFormat> textureFormatToGLFormat = {
		{ Display::TextureFormat::RGBA8, { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}},
		// { Display::TextureFormat::BGRA8, { GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE}},
		{ Display::TextureFormat::RGBA4, { GL_RGBA8, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4}},
		{ Display::TextureFormat::RGB8, { GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE}},
	};

	DisplayOpenGL::DisplayOpenGL() 
	  : mDrawables(this)
	{}
	DisplayOpenGL::~DisplayOpenGL() {}

	void DisplayOpenGL::contextLost() {
		std::cerr << "Context Loss" << std::endl;
	}
	void DisplayOpenGL::resolutionChanged(unsigned int w, unsigned int h) {
		std::cerr << "Resolution Changed " << w << "x" << h << std::endl;
		mWidth = w;
		mHeight = h;
	}
	std::string DisplayOpenGL::getDebugInformationString() {
		std::cerr << "getDebugInformationString" << std::endl;
		return "calm::DisplayOpenGL";
	}
	void DisplayOpenGL::init() {

		// TODO: Currently renders to whatever the current context is
		//       managed externally by the window setup / engine.
		//       This will probably need to change, when async texture
		//       uploads are added.

		mGLVendor = getString(GL_VENDOR);
		mGLRenderer = getString(GL_RENDERER);
		mGLVersion = getString(GL_VERSION);
		mGLProfileMask = getInt(GL_CONTEXT_PROFILE_MASK);

		if( auto c = gluGetString(GLU_VERSION) ) {
			mGLUVersion = reinterpret_cast<const char*>(c);
		}

		mNumTextureUnits = getInt(GL_MAX_TEXTURE_UNITS);
		mNumTextureUnitsCombined = getInt(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS);
		mTextureUnitForTexUploads = mNumTextureUnitsCombined - 1;
		mMaxTextureSize = getInt(GL_MAX_TEXTURE_SIZE);

		glGenVertexArrays(1, &mVAO);
		glBindVertexArray(mVAO);

		// TODO !!!
		// Need to sort out the whole depth approach. Depth testing
		// is going to be needed if we want to batch lots of sprites
		// together at least.
		// glEnable(GL_DEPTH_TEST);
		glDisable(GL_DEPTH_TEST);

		glEnable(GL_BLEND);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		glDepthRange(0.0f, 1.0f);

		// TODO: Need to see what the defaults are for all of this..
		// Previous GL4 prototype had this as default
		// glBlendEquation(GL_FUNC_ADD);
		// glBlendFuncSeparate(GL_ONE, GL_ZERO, GL_ONE, GL_ZERO);
		// But the usual blend mode we use in GL actually works better - Because we actually __need__ alpha blending for sprites
		// Blend modes are part of the tracked render state somehow, and affect how texture rendering is performed in a big way
		// Likely some of this will need to be done in-shader under gl 3/4
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// TODO: I needed this in the GL4 prototype - Really?
		// - Vertex ordering to drawQuads does seem to be variable,
		//   with conflicting docs on order between Sprite and RageDisplay_OGL
		// - Or just because the arrows can rotate? They have an outer surface though
		glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);

		// mRenderer.init();
	}

	void DisplayOpenGL::doDraw(std::vector<std::shared_ptr<Drawable>>&& d) {
		glViewport(0, 0, mWidth, mHeight);

		for( auto& dd : d ) {
			// glClear(GL_DEPTH_BUFFER_BIT);
			dd->draw();
		}

		// glClearColor(1.0, 0.0, 1.0, 1.0);
		// glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// TODO: swap buffers and handle sync - Probably needs to be in here,
		//       because we need to flip here and then handle a sync via glFinish or fences.
		//       But perhaps calling back out to the rageadapter isn't great, since that
		//       means we're bound to 'rage' in some form.
		// TODO: For now, draw comes through the adapter, and the adapter handles the sync,
		//       leaving the Display more like the Renderer in the previous RageDisplay_GL4
		//       prototype.
	}

	std::uintptr_t DisplayOpenGL::createTexture(
		TextureFormat format,
		uint8_t* pixels,
		uint32_t w, uint32_t h,
		uint32_t pitch, uint32_t bytesPerPixel,
		bool generateMipMaps){
		if( !pixels ) return 0;

		auto texFormat = textureFormatToGLFormat.find(format);
		if( texFormat == textureFormatToGLFormat.end() )
		{
			return 0;
		}

		GLuint tex;
		glGenTextures(1, &tex);
		// Use a single texture unit for uploaads - Probably not needed
		// here, but carried over from previous GL4 prototype
		glActiveTexture(GL_TEXTURE0 + mTextureUnitForTexUploads);
		glBindTexture(GL_TEXTURE_2D, tex);

		// TODO: Texture filtering settings and similar
		// - In RageDisplay these are based on the last call to setTextureMode
		//   or whatever, but will need to be managed in some form.
		// - Some of these are per-texture, and some are per-sampler,
		//   so the management crosses over between DisplayOpenGL
		//   and the Drawable implementation.
		// - Probably best to track the needed mode just as a global thing
		//   and copy that over to the Drawables, similar to the GL4 state
		//   tracking stuff but a lot simpler.
		// - If all of that means it's slow so be it, we can always
		//   rework the render path to not be terrible, once the basics
		//   are in place.

		// Note: glTexParameteri affects the state of the texture currently bound
		// to the texture unit - In core profile, the only texture unit state is
		// which texture is currently bound.
		if (generateMipMaps)
		{
			// if (mWindow->GetActualVideoModeParams().bTrilinearFiltering)
			// {
			// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			// 	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// 	s.textureState[tex].minFilter = GL_LINEAR_MIPMAP_LINEAR;
			// 	s.textureState[tex].magFilter = GL_LINEAR;
			// }
			// else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / bytesPerPixel);
		glTexImage2D(GL_TEXTURE_2D, 0, texFormat->second.internalfmt,
			w, h, 0,
			texFormat->second.format, texFormat->second.type,
			pixels
		);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		if (generateMipMaps) {
			// TODO: Slow - Just make sure it's not happening for movies..
			glGenerateMipmap(GL_TEXTURE_2D);
		}

		return tex;
	}

	void DisplayOpenGL::deleteTexture( std::uintptr_t iTexHandle ) {
		GLuint t = iTexHandle;
		glDeleteTextures(1, &t);
	}
}
