#include "global.h"

#include "CalmDisplayOpenGL.h"
#include "RageFile.h"
#include "RageUtil.h"

#include "RageTypes.h"

#include <regex>

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
			case CullMode::CULL_NONE:
				glDisable(GL_CULL_FACE);
				break;
			case CullMode::CULL_FRONT:
				glEnable(GL_CULL_FACE);
				glCullFace(GL_FRONT);
				break;
			case CullMode::CULL_BACK:
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
			case ZTestMode::ZTEST_WRITE_ON_FAIL:
				glDepthFunc(GL_GREATER);
				break;
			case ZTestMode::ZTEST_WRITE_ON_PASS:
				glDepthFunc(GL_LEQUAL);
				break;
			case ZTestMode::ZTEST_OFF:
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
		if (state.blendMode == BlendMode::BLEND_INVERT_DEST)
			blendEq = GL_FUNC_SUBTRACT;
		else if (state.blendMode == BlendMode::BLEND_SUBTRACT)
			blendEq = GL_FUNC_REVERSE_SUBTRACT;

		int iSourceRGB, iDestRGB;
		int iSourceAlpha = GL_ONE, iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
		switch (state.blendMode)
		{
		case BlendMode::BLEND_NORMAL:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BlendMode::BLEND_ADD:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE;
			break;
		case BlendMode::BLEND_SUBTRACT:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BlendMode::BLEND_MODULATE:
			iSourceRGB = GL_ZERO; iDestRGB = GL_SRC_COLOR;
			break;
		case BlendMode::BLEND_COPY_SRC:
			iSourceRGB = GL_ONE; iDestRGB = GL_ZERO;
			iSourceAlpha = GL_ONE; iDestAlpha = GL_ZERO;
			break;
		case BlendMode::BLEND_ALPHA_MASK:
			iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO; iDestAlpha = GL_SRC_ALPHA;
			break;
		case BlendMode::BLEND_ALPHA_KNOCK_OUT:
			iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
			iSourceAlpha = GL_ZERO; iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
			break;
		case BlendMode::BLEND_ALPHA_MULTIPLY:
			iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ZERO;
			break;
		case BlendMode::BLEND_WEIGHTED_MULTIPLY:
			/* output = 2*(dst*src).  0.5,0.5,0.5 is identity; darker colors darken the image,
			* and brighter colors lighten the image. */
			iSourceRGB = GL_DST_COLOR; iDestRGB = GL_SRC_COLOR;
			break;
		case BlendMode::BLEND_INVERT_DEST:
			/* out = src - dst.  The source color should almost always be #FFFFFF, to make it "1 - dst". */
			iSourceRGB = GL_ONE; iDestRGB = GL_ONE;
			break;
		case BlendMode::BLEND_NO_EFFECT:
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


	std::shared_ptr<ShaderProgram> DisplayOpenGL::getShader(ShaderParameters params, bool allowCompile)
	{
		auto shaderIt = std::find_if(mShaders.begin(), mShaders.end(), [&params](auto x) { return x.first == params; });
		if( shaderIt != mShaders.end() ) return shaderIt->second;

		// validate
		auto texture0Enabled = params.features.find(ShaderFeature::TextureUnit0) != params.features.end();
		auto texture1Enabled = params.features.find(ShaderFeature::TextureUnit1) != params.features.end();
		auto texture2Enabled = params.features.find(ShaderFeature::TextureUnit2) != params.features.end();
		auto texture3Enabled = params.features.find(ShaderFeature::TextureUnit3) != params.features.end();
		switch( params.base )
		{
			case ShaderBase::Sprite:
				ASSERT_M(texture0Enabled, "Shader requires texture 0");
				ASSERT_M(!texture1Enabled, "Shader doesn't support texture 1");
				ASSERT_M(!texture2Enabled, "Shader doesn't support texture 2");
				ASSERT_M(!texture3Enabled, "Shader doesn't support texture 3");
				break;
			case ShaderBase::MultiTexture:
				ASSERT_M(texture0Enabled, "Shader requires texture 0");
				ASSERT_M(texture0Enabled, "Shader requires texture 1");
				ASSERT_M(texture0Enabled, "Shader requires texture 2");
				ASSERT_M(texture0Enabled, "Shader requires texture 3");
				break;
			default:
				FAIL_M("Unknown base shader");
		}
		
		switch( params.effectMode )
		{
			case EffectMode::EffectMode_Unpremultiply:
			case EffectMode::EffectMode_DistanceField:
				ASSERT_M(texture0Enabled, "Effect mode requires texture 0");
				break;
			case EffectMode::EffectMode_ColorBurn:
			case EffectMode::EffectMode_ColorDodge:
			case EffectMode::EffectMode_HardMix:
			case EffectMode::EffectMode_Overlay:
			case EffectMode::EffectMode_Screen:
			case EffectMode::EffectMode_VividLight:
			case EffectMode::EffectMode_YUYV422:
				ASSERT_M(texture0Enabled, "Effect mode requires texture 0");
				ASSERT_M(texture1Enabled, "Effect mode requires texture 1");
				break;
			case EffectMode::EffectMode_Normal:
				break;
			default:
				FAIL_M("Unknown effect mode requested");
				break;
		}

		// Load shader source
		std::string vertSrc;
		std::string fragSrc;
		switch( params.base )
		{
			case ShaderBase::Sprite:
				vertSrc = RageFile::load("Data/Shaders/calm/OpenGL/sprite.vert");
				fragSrc = RageFile::load("Data/Shaders/calm/OpenGL/sprite.frag");
				break;
			case ShaderBase::MultiTexture:
				vertSrc = RageFile::load("Data/Shaders/calm/OpenGL/multitexture.vert");
				fragSrc = RageFile::load("Data/Shaders/calm/OpenGL/multitexture.frag");
				break;
			default:
				FAIL_M("Unknown base shader requested");
				break;
		}

		// Enable features
		auto uniformStr = [](auto i) { return std::string("uniform sampler2D texture") + std::to_string(i) + ";\n" \
			                                + std::string("uniform bool texture") + std::to_string(i) + "enabled;"; };
		if( texture0Enabled ) fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_0_UNIFORMS##" ), uniformStr(0));
		if( texture1Enabled ) fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_1_UNIFORMS##" ), uniformStr(1));
		if( texture2Enabled ) fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_2_UNIFORMS##" ), uniformStr(2));
		if( texture3Enabled ) fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_3_UNIFORMS##" ), uniformStr(3));

		static std::string texModeNone = RageFile::load("Data/Shaders/calm/OpenGL/texturemode/none.frag");
		static std::string texModeModulate = RageFile::load("Data/Shaders/calm/OpenGL/texturemode/modulate.frag");
		static std::string texModeGlow = RageFile::load("Data/Shaders/calm/OpenGL/texturemode/glow.frag");
		static std::string texModeAdd = RageFile::load("Data/Shaders/calm/OpenGL/texturemode/add.frag");
		auto texID = [](auto s, auto i) { 
			return std::regex_replace(s, std::regex( "\\##TEXTURE_UNIT##" ), std::to_string(i));
		};

		switch(params.textureMode0)
		{
			case TextureMode::TextureMode_Modulate:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_0_MODE##" ), texID(texModeModulate, 0));
				break;
			case TextureMode::TextureMode_Glow:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_0_MODE##" ), texID(texModeGlow, 0));
				break;
			case TextureMode::TextureMode_Add:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_0_MODE##" ), texID(texModeAdd, 0));
				break;
			default:
				FAIL_M("Unknown texture mode");
		}
		switch(params.textureMode1)
		{
			case TextureMode::TextureMode_Modulate:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_1_MODE##" ), texID(texModeModulate, 1));
				break;
			case TextureMode::TextureMode_Glow:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_1_MODE##" ), texID(texModeGlow, 1));
				break;
			case TextureMode::TextureMode_Add:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_1_MODE##" ), texID(texModeAdd, 1));
				break;
			default:
				FAIL_M("Unknown texture mode");
		}
		switch(params.textureMode2)
		{
			case TextureMode::TextureMode_Modulate:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_2_MODE##" ), texID(texModeModulate, 2));
				break;
			case TextureMode::TextureMode_Glow:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_2_MODE##" ), texID(texModeGlow, 2));
				break;
			case TextureMode::TextureMode_Add:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_2_MODE##" ), texID(texModeAdd, 2));
				break;
			default:
				FAIL_M("Unknown texture mode");
		}
		switch(params.textureMode3)
		{
			case TextureMode::TextureMode_Modulate:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_3_MODE##" ), texID(texModeModulate, 3));
				break;
			case TextureMode::TextureMode_Glow:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_3_MODE##" ), texID(texModeGlow, 3));
				break;
			case TextureMode::TextureMode_Add:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##TEXTURE_3_MODE##" ), texID(texModeAdd, 3));
				break;
			default:
				FAIL_M("Unknown texture mode");
		}

		switch(params.effectMode)
		{
			case EffectMode::EffectMode_Normal:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/normal.frag"));
				break;
			case EffectMode::EffectMode_Unpremultiply:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/unpremultiply.frag"));
				break;
			case EffectMode::EffectMode_DistanceField:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/distancefield.frag"));
				break;
			case EffectMode::EffectMode_ColorBurn:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/colourburn.frag"));
				break;
			case EffectMode::EffectMode_ColorDodge:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/colourdodge.frag"));
				break;
			case EffectMode::EffectMode_HardMix:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/hardmix.frag"));
				break;
			case EffectMode::EffectMode_Overlay:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/overlay.frag"));
				break;
			case EffectMode::EffectMode_Screen:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/screen.frag"));
				break;
			case EffectMode::EffectMode_VividLight:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/vividlight.frag"));
				break;
			case EffectMode::EffectMode_YUYV422:
				fragSrc = std::regex_replace(fragSrc, std::regex( "\\##EFFECT_MODE##" ), 
				RageFile::load("Data/Shaders/calm/OpenGL/effectmode/yuyv422.frag"));
				break;
			default:
				FAIL_M("Unknown effect mode requested");
				break;
		}

		auto shader = std::make_shared<ShaderProgram>(vertSrc, fragSrc);
		std::string err;
		ASSERT_M(shader->compile(err), err.c_str());
		switch(params.base)
		{
			case ShaderBase::Sprite:
				shader->initUniforms(ShaderProgram::UniformType::Sprite);
				break;
			case ShaderBase::MultiTexture:
				shader->initUniforms(ShaderProgram::UniformType::MultiTexture);
				break;
			default:
				FAIL_M("Unknown uniform init call for base shader");
				break;
		}
		mShaders.push_back({params, shader});
		return shader;
	}
}
