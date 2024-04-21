
#include "CalmDisplayOpenGL.h"

// TODO: Copied from RageUtil.h - Used for zBias, but why?
#define SCALE(x, l1, h1, l2, h2) (((x) - (l1)) * ((h2) - (l2)) / ((h1) - (l1)) + (l2))

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
	void DisplayOpenGL::init(InitParameters p) {

		mTrilinearFilteringEnabled = p.trilinearFilteringEnabled;

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
		// glDepthMask(GL_TRUE); // TODO: ZWrite enable/disable?
		glDepthRange(0.0f, 1.0f);

		setRenderState({});
	}

	void DisplayOpenGL::doDraw(std::vector<std::shared_ptr<Drawable>>&& d) {
		glViewport(0, 0, mWidth, mHeight);

		for( auto& dd : d ) {
			// glClear(GL_DEPTH_BUFFER_BIT);
			dd->draw(this);
		}

		// CALM TODO - Frame sync / vsync / glfinish stuff
		// TODO: swap buffers and handle sync - Probably needs to be in here,
		//       because we need to flip here and then handle a sync via glFinish or fences.
		//       But perhaps calling back out to the rageadapter isn't great, since that
		//       means we're bound to 'rage' in some form.
		// TODO: For now, draw comes through the adapter, and the adapter handles the sync,
		//       leaving the Display more like the Renderer in the previous RageDisplay_GL4
		//       prototype.
	}

	void DisplayOpenGL::doSync() {
		if (!frameSyncUsingFences)
		{
			// Some would advise against glFinish(), ever. Those people don't realize
			// the degree of freedom GL hosts are permitted in queueing commands.
			// If left to its own devices, the host could lag behind several frames' worth
			// of commands.
			// glFlush() only forces the host to not wait to execute all commands
			// sent so far; it does NOT block on those commands until they finish.
			// glFinish() blocks. We WANT to block. Why? This puts the engine state
			// reflected by the next frame as close as possible to the on-screen
			// appearance of that frame.
			glFinish();
		}
		else
		{
			// Hey geefr here, I'm one of 'those people' mentioned above.
			// glFinish is terrible, and should never be called in any circumstances.
			// 
			// Instead use fences to wait for frame N-x to be rendered, rather than an
			// outright stall. This is arguably less predictable, but allows the cpu
			// to continue scheduling work for the next frame.
			//
			// Does that mean we're rendering a little behind 'now'? Yes it does.
			// The hope here is that we'll be a stable N-x frames behind, and if
			// the player really cares that much they can set their visual offset
			// accordingly.
			frameSyncFences.push_back(glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0));
			if (frameSyncFences.size() >= frameSyncDesiredFramesInFlight)
			{
				GLsync fence = frameSyncFences.front();
				frameSyncFences.pop_front();
				// Wait up to 33ms for the fence - The result doesn't matter.
				// If we can't maintain 30fps then the visual sync
				// won't matter much to the player, and we're probably struggling
				// so much that we should let the gpu do whatever it likes.
				//
				// Ideally though we want to see GL_CONDITION_SATISFIED, meaning we
				// rendered fast enough to need to wait, and then waited until the
				// end of the previous frame (assuming 1-frames-in-flight).
				// Waiting here also glflush()es the current frame.
				glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 33000000);
			}
		}
	}

	void DisplayOpenGL::setRenderState(RenderState state) {
		// TODO: this will happen frequently, it shouldn't (!!!)
		switch(state.cullMode)
		{
			case CullMode::None:
				glDisable(GL_CULL_FACE);
				break;
			case CullMode::Front:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			case CullMode::Back:
			default:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_BACK);
				break;
		}

		if( state.zWrite ) {
			glDepthMask(GL_TRUE);
		} else {
			glDepthMask(GL_FALSE);
		}
		switch (state.zTestMode)
		{
			case ZTestMode::WriteOnFail:
				glDepthFunc(GL_GREATER);
				break;
			case ZTestMode::WriteOnPass:
				glDepthFunc(GL_LEQUAL);
				break;
			case ZTestMode::Off:
			default:
				glDepthFunc(GL_ALWAYS);
				break;
		}

		float fNear = SCALE(state.zBias, 0.0f, 1.0f, 0.05f, 0.0f);
		float fFar = SCALE(state.zBias, 0.0f, 1.0f, 1.0f, 0.95f);
		glDepthRange(fNear, fFar);

		// TODO: Shouldn't do this here, but it's needed for some rendering
		//       to work. Awkwardly it's not part of the drawable / a draw command
		//       and will prevent neat batching / instancing of drawables.
		if( state.clearZBuffer ) {
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		GLenum blendEq = GL_FUNC_ADD;
		if (state.blendMode == BlendMode::InvertDest)
			blendEq = GL_FUNC_SUBTRACT;
		else if (state.blendMode == BlendMode::Subtract)
			blendEq = GL_FUNC_REVERSE_SUBTRACT;

		int iSourceRGB, iDestRGB;
		int iSourceAlpha = GL_ONE, iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
		switch (state.blendMode)
		{
		case BlendMode::Normal:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BlendMode::Add:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE;
			break;
		case BlendMode::Subtract:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BlendMode::Modulate:
			iSourceRGB = GL_ZERO; iDestRGB = GL_SRC_COLOR;
			break;
		case BlendMode::CopySrc:
			iSourceRGB = GL_ONE; iDestRGB = GL_ZERO;
			iSourceAlpha = GL_ONE; iDestAlpha = GL_ZERO;
			break;
		case BlendMode::AlphaMask:
			iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO; iDestAlpha = GL_SRC_ALPHA;
			break;
		case BlendMode::AlphaKnockOut:
			iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO; iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BlendMode::AlphaMultiply:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ZERO;
			break;
		case BlendMode::WeightedMultiply:
			/* output = 2*(dst*src).  0.5,0.5,0.5 is identity; darker colors darken the image,
			* and brighter colors lighten the image. */
			iSourceRGB = GL_DST_COLOR; iDestRGB = GL_SRC_COLOR;
			break;
		case BlendMode::InvertDest:
			/* out = src - dst.  The source color should almost always be #FFFFFF, to make it "1 - dst". */
			iSourceRGB = GL_ONE; iDestRGB = GL_ONE;
			break;
		case BlendMode::NoEffect:
		default:
			iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO; iDestAlpha = GL_ONE;
			break;
		}
		glBlendEquation(blendEq);

		// GL 4.0 or ext
		glBlendFuncSeparate(iSourceRGB, iDestRGB, iSourceAlpha, iDestAlpha);
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
			if (mTrilinearFilteringEnabled)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			else
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

	
	void DisplayOpenGL::setTextureWrapping( uintptr_t texture, bool wrap ) {
		glActiveTexture(GL_TEXTURE0 + mTextureUnitForTexUploads);
		glBindTexture(GL_TEXTURE_2D, texture);
		GLenum mode = wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE;
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mode );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mode );
	}
	void DisplayOpenGL::setTextureFiltering( uintptr_t texture, bool filter ) {
		glActiveTexture(GL_TEXTURE0 + mTextureUnitForTexUploads);
		glBindTexture(GL_TEXTURE_2D, texture);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);

		GLint iMinFilter;
		if (filter)
		{
			GLint iWidth1 = -1;
			GLint iWidth2 = -1;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &iWidth1);
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &iWidth2);
			if (iWidth1 > 1 && iWidth2 != 0)
			{
				/* Mipmaps are enabled. */
				if (mTrilinearFilteringEnabled)
					iMinFilter = GL_LINEAR_MIPMAP_LINEAR;
				else
					iMinFilter = GL_LINEAR_MIPMAP_NEAREST;
			}
			else
			{
				iMinFilter = GL_LINEAR;
			}
		}
		else
		{
			iMinFilter = GL_NEAREST;
		}

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, iMinFilter );
	}
}
