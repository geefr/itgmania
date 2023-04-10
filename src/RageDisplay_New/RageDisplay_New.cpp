#include "global.h"

#include "RageDisplay.h"
#include "RageDisplay_New.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "RageMath.h"
#include "RageTypes.h"
#include "RageUtil.h"
#include "RageSurface.h"
#include "DisplaySpec.h"
#include "RageDisplay_New_Geometry.h"

#include <GL/glew.h>
#ifdef WINNT
# include <GL/wglew.h>
#endif
#include <RageFile.h>

#include <algorithm>
#include <RageTextureManager.h>

namespace {
	const bool enableGLDebugGroups = false;
	const bool periodicShaderReload = true;

	class GLDebugGroup
	{
	public:
		GLDebugGroup(std::string n)
		{
			if (enableGLDebugGroups) {
				glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, n.size(), n.data());
			}
		}
		~GLDebugGroup()
		{
			if (enableGLDebugGroups) {
				glPopDebugGroup();
			}
		}
	};

	static RageDisplay::RagePixelFormatDesc PIXEL_FORMAT_DESC[NUM_RagePixelFormat] = {
		{
			/* R8G8B8A8 */
			32,
			{ 0xFF000000,
			  0x00FF0000,
			  0x0000FF00,
			  0x000000FF }
		}, {
			/* R4G4B4A4 */
			16,
			{ 0xF000,
			  0x0F00,
			  0x00F0,
			  0x000F },
		}, {
			/* R5G5B5A1 */
			16,
			{ 0xF800,
			  0x07C0,
			  0x003E,
			  0x0001 },
		}, {
			/* R5G5B5 */
			16,
			{ 0xF800,
			  0x07C0,
			  0x003E,
			  0x0000 },
		}, {
			/* R8G8B8 */
			24,
			{ 0xFF0000,
			  0x00FF00,
			  0x0000FF,
			  0x000000 }
		}, {
			/* Paletted */
			8,
			{ 0,0,0,0 } /* N/A */
		}, {
			/* B8G8R8A8 */
			24,
			{ 0x0000FF,
			  0x00FF00,
			  0xFF0000,
			  0x000000 }
		}, {
			/* A1B5G5R5 */
			16,
			{ 0x7C00,
			  0x03E0,
			  0x001F,
			  0x8000 },
		}
	};

	// TODO: This mapping is probably fine, but maybe we should texture cache instead
	//       in a more sensible format, and then not need to re-convert textures on
	//       the fly constantly?
	/* Used for both texture formats and surface formats. For example,
	 * it's fine to ask for a RagePixelFormat_RGB5 texture, but to supply a surface matching
	 * RagePixelFormat_RGB8. OpenGL will simply discard the extra bits.
	 *
	 * It's possible for a format to be supported as a texture format but not as a
	 * surface format. For example, if packed pixels aren't supported, we can still
	 * use GL_RGB5_A1, but we'll have to convert to a supported surface pixel format
	 * first. It's not ideal, since we'll convert to RGBA8 and OGL will convert back,
	 * but it works fine.
	 */
	struct RagePixelFormatToGLFormat {
		GLenum internalfmt;
		GLenum format;
		GLenum type;
	} const ragePixelFormatToGLFormat[NUM_RagePixelFormat] = {
		{
			/* R8G8B8A8 */
			GL_RGBA8,
			GL_RGBA,
			GL_UNSIGNED_BYTE,
		}, {
			/* R8G8B8A8 */
			GL_RGBA8,
			GL_BGRA,
			GL_UNSIGNED_BYTE,
		}, {
			/* B4G4R4A4 */
			GL_RGBA4,
			GL_RGBA,
			GL_UNSIGNED_SHORT_4_4_4_4,
		}, {
			/* B5G5R5A1 */
			GL_RGB5_A1,
			GL_RGBA,
			GL_UNSIGNED_SHORT_5_5_5_1,
		}, {
			/* B5G5R5 */
			GL_RGB5,
			GL_RGBA,
			GL_UNSIGNED_SHORT_5_5_5_1,
		}, {
			/* B8G8R8 */
			GL_RGB8,
			GL_RGB,
			GL_UNSIGNED_BYTE,
		}, {
			/* Paletted */
			GL_COLOR_INDEX8_EXT,
			GL_COLOR_INDEX,
			GL_UNSIGNED_BYTE,
		}, {
			/* B8G8R8 */
			GL_RGB8,
			GL_BGR,
			GL_UNSIGNED_BYTE,
		}, {
			/* A1R5G5B5 (matches D3DFMT_A1R5G5B5) */
			GL_RGB5_A1,
			GL_BGRA,
			GL_UNSIGNED_SHORT_1_5_5_5_REV,
		}, {
			/* X1R5G5B5 */
			GL_RGB5,
			GL_BGRA,
			GL_UNSIGNED_SHORT_1_5_5_5_REV,
		}
	};
}

RageDisplay_New::FrameBuffer::FrameBuffer(GLuint width, GLuint height, GLint tempTexUnit)
	: mWidth(width)
	, mHeight(height)
{
	glGenFramebuffers(1, &mFBO);
	bindRenderTarget();

	glGenTextures(1, &mTex);
	glActiveTexture(GL_TEXTURE0 + tempTexUnit);
	glBindTexture(GL_TEXTURE_2D, mTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTex, 0);

	glGenRenderbuffers(1, &mDepthRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, mDepthRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mDepthRBO);

	ASSERT_M(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "Failed to create FrameBuffer");
	unbindRenderTarget();
}

RageDisplay_New::FrameBuffer::~FrameBuffer()
{
	glDeleteRenderbuffers(1, &mDepthRBO);
	glDeleteTextures(1, &mTex);
	glDeleteFramebuffers(1, &mFBO);
}

void RageDisplay_New::FrameBuffer::bindRenderTarget()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
}

void RageDisplay_New::FrameBuffer::unbindRenderTarget()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RageDisplay_New::FrameBuffer::bindTexture(GLint texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, mTex);
}

RageDisplay_New::RageDisplay_New()
{
	LOG->MapLog("renderer", "Current renderer: new");
}

RString RageDisplay_New::Init(const VideoModeParams& p, bool /* bAllowUnacceleratedRenderer */)
{
	// Switch the gl context init to use the non-ancient approach
	// and yeet compatibility features out the window
	LowLevelWindow::useNewOpenGLContextCreation = true;
	LowLevelWindow::newOpenGLRequireCoreProfile = true;
	LowLevelWindow::newOpenGLContextCreationAcceptedVersions = {
		{4, 6},
		{4, 5},
		{4, 4},
		{4, 3},
		{4, 2},
		{4, 1},
		{4, 0},
	};

	mWindow = LowLevelWindow::Create();

	bool needReloadTextures = false;
	auto err = SetVideoMode(p, needReloadTextures);
	if (!err.empty())
	{
		return err;
	}

	if (mWindow->IsSoftwareRenderer(err))
	{
		return "Hardware accelerated rendering not available: " + err;
	}

	mWindow->LogDebugInformation();
	LOG->Info("OGL Vendor: %s", glGetString(GL_VENDOR));
	LOG->Info("OGL Renderer: %s", glGetString(GL_RENDERER));
	LOG->Info("OGL Version: %s", glGetString(GL_VERSION));
	GLint profileMask;
	glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &profileMask);
	LOG->Info("OGL Profile: %s", profileMask & GL_CONTEXT_CORE_PROFILE_BIT ? "CORE" : "COMPATIBILITY");

	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &mNumTextureUnits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &mNumTextureUnitsCombined);

	glGetFloatv(GL_LINE_WIDTH_RANGE, mLineWidthRange);
	glGetFloatv(GL_POINT_SIZE_RANGE, mPointSizeRange);

	mTextureUnitForTexUploads = mNumTextureUnitsCombined - 1;
	mTextureUnitForFBOUploads = mNumTextureUnitsCombined - 2;
	mTextureUnitForFlipFlopRender = mNumTextureUnitsCombined - 3;

	LoadShaderPrograms();

	initLights();

	flipflopRenderInit();

	// Depth testing is always enabled, even if depth write is not
	glEnable(GL_DEPTH_TEST);

	// TODO
	// - MSAA
	//   - Either globally on context
	//   - Or since we have flipflop now, MSAA there and add a postprocessing shader >:D

	return {};
}

RageDisplay_New::~RageDisplay_New()
{
	flipflopRenderDeInit();
	ClearAllTextures();
	mActiveShaderProgram.first = ShaderName::Invalid;
	mActiveShaderProgram.second = {};
	mShaderPrograms.clear();
	if (mWindow)
	{
		delete mWindow;
	}
}

void RageDisplay_New::ResolutionChanged()
{
	GLDebugGroup g("ResolutionChanged");
	// TODO: What goes here? related to offscreen rendering and similar
	//       and if there's state tracking we need to reset.
	//       May also need to reload all shaders - Is this a context loss?

	RageDisplay::ResolutionChanged();

	// TODO: With this, we could trivially have a different render resolution
	auto width = mWindow->GetActualVideoModeParams().windowWidth;
	auto height = mWindow->GetActualVideoModeParams().windowHeight;

	if (width > 0 && height > 0)
	{
		// mFlip.reset(new FrameBuffer(width, height, mTextureUnitForFBOUploads));
		// mFlop.reset(new FrameBuffer(width, height, mTextureUnitForFBOUploads));
	}
}

bool RageDisplay_New::SupportsTextureFormat(RagePixelFormat fmt, bool realtime)
{
	// TODO: Ported from RageDisplay_Legacy
	  /* If we support a pixfmt for texture formats but not for surface formats, then
	   * we'll have to convert the texture to a supported surface format before uploading.
	   * This is too slow for dynamic textures. */
	if (realtime && !SupportsSurfaceFormat(fmt))
		return false;

	switch (ragePixelFormatToGLFormat[fmt].format)
	{
	case GL_COLOR_INDEX:
		return false; // TODO: New renderer doesn't support palleting yet. Why would anyone use those in 2023? (Let's find out when it breaks)
		  // return glColorTableEXT && glGetColorTableParameterivEXT;
	default:
		// Boldly assume that because we're using newer GL, we support all formats
		return true;
	}
}

/*
 * Although we pair texture formats (eg. GL_RGB8) and surface formats
 * (pairs of eg. GL_RGB8,GL_UNSIGNED_SHORT_5_5_5_1), it's possible for
 * a format to be supported for a texture format but not a surface
 * format.  This is abstracted, so you don't need to know about this
 * as a user calling CreateTexture.
 *
 * One case of this is if packed pixels aren't supported.  We can still
 * use 16-bit color modes, but we have to send it in 32-bit.  Almost
 * everything supports packed pixels.
 *
 * Another case of this is incomplete packed pixels support.  Some implementations
 * neglect GL_UNSIGNED_SHORT_*_REV.
 */
bool RageDisplay_New::SupportsSurfaceFormat(RagePixelFormat fmt)
{
	// TODO: Ported from RageDisplay_Legacy
	// TODO: Yeah all this texture format gubbins needs a rewrite, along with threaded texture uploads for video frames
	return true;
}

void RageDisplay_New::LoadShaderPrograms(bool failOnError)
{
  mShaderPrograms.clear();

  // This shader supports both sprite and model rendering
  // But if it's loaded twice, it should reduce the number of state changes
  // assuming that most uniforms remain the same across multiple uses of
  // the shader.
  // TODO: Shader preprocessing could cut down the megashader somewhat..

  mShaderPrograms[ShaderName::MegaShader] = {
		"Data/Shaders/GLSL_400/megashader.vert",
		"Data/Shaders/GLSL_400/megashader.frag",
  };

  // TODO: For some reason enabling the 2nd instance of this shader breaks everything.
  //       Perhaps that also happens for any other shader too - Some assumed state between them somehow?
  //mShaderPrograms[ShaderName::MegaShaderCompiledGeometry] = {
		//"Data/Shaders/GLSL_400/megashader.vert",
		//"Data/Shaders/GLSL_400/megashader.frag",
  //};

  for (auto& p : mShaderPrograms)
  {
	  auto success = p.second.init();
		ASSERT_M(!failOnError || success, "Failed to compile shader program");
  }

  // TODO: Doesn't work, but bring it back as a quad/preprocessing shader at some point
  // TODO: Will need a common base class for shader programs / interfaces..
	/*LoadShaderProgram(ShaderName::FlipFlopFinal,
		"Data/Shaders/GLSL_400/flipflopfinal.vert",
		"Data/Shaders/GLSL_400/flipflopfinal.frag",
		failOnError);*/
}

bool RageDisplay_New::UseProgram(ShaderName name)
{
	if (name == mActiveShaderProgram.first)
	{
		return true;
  }

	auto it = mShaderPrograms.find(name);
	if (it == mShaderPrograms.end())
	{
		LOG->Warn("Invalid shader program requested: %i", name);
		return false;
	}

  it->second.bind();
  mActiveShaderProgram.first = it->first;
  mActiveShaderProgram.second = it->second;
	return true;
}

void RageDisplay_New::initLights()
{
	// https://registry.khronos.org/OpenGL-Refpages/gl2.1/xhtml/glLight.xml
	for (auto i = 1; i < RageDisplay_New_ShaderProgram::MaxLights; ++i)
	{
		auto& l = mLights[i];
		l.enabled = false;
		l.ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
		l.diffuse = { 0.0f, 0.0f, 0.0f, 1.0f };
		l.specular = { 0.0f, 0.0f, 0.0f, 1.0f };
		l.position = { 0.0f, 0.0f, 1.0f , 0.0f };
	}

	auto& light0 = mLights[0];
	light0.enabled = true;
	light0.ambient = { 0.0f, 0.0f, 0.0f, 1.0f };
	light0.diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
	light0.specular = { 1.0f, 1.0f, 1.0f, 1.0f };
	light0.position = { 0.0f, 0.0f, 1.0f, 0.0f };
}

void RageDisplay_New::SetShaderUniforms()
{
	// Matrices
	RageMatrixMultiply(&mMatrices.projection, GetCentering(), GetProjectionTop());

	// TODO: Related to render to texture
	/*if (g_bInvertY)
	{
		RageMatrix flip;
		RageMatrixScale(&flip, +1, -1, +1);
		RageMatrixMultiply(&projection, &flip, &projection);
	}*/

	RageMatrixMultiply(&mMatrices.modelView, GetViewTop(), GetWorldTop());
	mMatrices.texture = *GetTextureTop();

	auto& s = mActiveShaderProgram.second;
	s.setUniformMatrices(mMatrices);
	for( auto i = 0; i < RageDisplay_New_ShaderProgram::MaxTextures; ++i )
	{
		s.setUniformTextureSettings(i, mTextureSettings[i]);
		s.setUniformTextureUnit(i, static_cast<TextureUnit>(mTextureUnits[i]));
	}
	s.setUniformMaterial(mMaterial);
	for (auto i = 0; i < RageDisplay_New_ShaderProgram::MaxLights; ++i)
	{
		s.setUniformLight(i, mLights[i]);
	}

	s.updateUniforms();
}

void RageDisplay_New::GetDisplaySpecs(DisplaySpecs& out) const
{
	out.clear();
	if (mWindow)
	{
		mWindow->GetDisplaySpecs(out);
	}
}

RString RageDisplay_New::TryVideoMode(const VideoModeParams& p, bool& newDeviceCreated)
{
	if (!mWindow)
	{
		return "Invalid Window";
	}

	auto err = mWindow->TryVideoMode(p, newDeviceCreated);
	if (!err.empty())
	{
		return err;
	}

	if (newDeviceCreated)
	{
		if (TEXTUREMAN)
		{
			/*for (auto& t : mTextures)
			{
				glDeleteTextures(1, &t);
			}
			mTextures.clear();*/

			TEXTUREMAN->InvalidateTextures();
		}

		for (auto& g : mCompiledGeometry)
		{
			g->contextLost();
		}

		for (auto& s : mShaderPrograms)
		{
			// TODO: Should invalidate before context loss surely? Perhaps we just have to re-init every time regardless
			s.second.invalidate();
			s.second.init();
		}
	}

	ResolutionChanged();

	return {};
}

RageSurface* RageDisplay_New::CreateScreenshot()
{
	/*const RagePixelFormatDesc &desc = PIXEL_FORMAT_DESC[RagePixelFormat_RGB8];
	RageSurface *image = CreateSurface(
		640, 480, desc.bpp,
		desc.masks[0], desc.masks[1], desc.masks[2], desc.masks[3] );

	memset( image->pixels, 0, 480*image->pitch );

	return image;*/

	return nullptr;
}

const RageDisplay::RagePixelFormatDesc* RageDisplay_New::GetPixelFormatDesc(RagePixelFormat pf) const
{
	ASSERT(pf >= 0 && pf < NUM_RagePixelFormat);
	return &PIXEL_FORMAT_DESC[pf];
}

bool RageDisplay_New::BeginFrame()
{
	// TODO: A hack to let me reload shaders without restarting the game
	if (periodicShaderReload)
	{
		static int i = 0;
		if (++i == 100)
		{
			auto previousShader = mActiveShaderProgram.first;
			LoadShaderPrograms(false);
			// ResolutionChanged();
			UseProgram(previousShader);

			i = 0;
		}
	}

	GLDebugGroup g("BeginFrame");

	auto width = mWindow->GetActualVideoModeParams().windowWidth;
	auto height = mWindow->GetActualVideoModeParams().windowHeight;

	// Clear all framebuffers, configure for first render
	/*mFlop->bindRenderTarget();
	glViewport(0, 0, width, height);
	SetZWrite(true);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	mFlop->bindTexture(mTextureUnitForFlipFlopRender);*/

	/*mFlip->bindRenderTarget();
	glViewport(0, 0, width, height);
	SetZWrite(true);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);*/

	glViewport(0, 0, width, height);
	SetZWrite(true);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	bool beginFrame = RageDisplay::BeginFrame();
	// TODO: Offscreen render target / FBOs
	/*if (beginFrame && UseOffscreenRenderTarget()) {
		offscreenRenderTarget->BeginRenderingTo(false);
	}*/

	return beginFrame;
}

void RageDisplay_New::flipflopFBOs()
{
	// mFlip.swap(mFlop);

	// mFlip->bindRenderTarget();
	// mFlop->bindTexture(mTextureUnitForFlipFlopRender);
}

void RageDisplay_New::flipflopRenderInit()
{
	//glCreateVertexArrays(1, &mFlipflopRenderVAO);
	//glBindVertexArray(mFlipflopRenderVAO);
	//glGenBuffers(1, &mFlipflopRenderVBO);
	//glBindBuffer(GL_ARRAY_BUFFER, mFlipflopRenderVBO);
	//const std::vector<float> verts = {
	//	-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	//   1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
	//   1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	//   1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
	//  -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
	//  -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
	//};
	//glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), reinterpret_cast<const void*>(3 * sizeof(float)));
	//glEnableVertexAttribArray(0);
	//glEnableVertexAttribArray(1);
}

void RageDisplay_New::flipflopRender()
{
	// All draw calls swap, so the final output is actually in flop, not flip

   // mFlip->unbindRenderTarget();
   // // mFlop->bindTexture(mTextureUnitForFlipFlopRender);
   // mFlip->bindTexture(mTextureUnitForFlipFlopRender);

   // auto width = mWindow->GetActualVideoModeParams().windowWidth;
   // auto height = mWindow->GetActualVideoModeParams().windowHeight;
	  //glViewport(0, 0, width, height);
	  //SetZWrite(true);
	  //glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	  //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	  //glBindVertexArray(mFlipflopRenderVAO);
	  //UseProgram(ShaderName::FlipFlopFinal);
   // // TODO: Don't need all of these just for postprocessing,
   // //       but might aswell have a consistent shader interface?
	  //SetShaderUniforms();
   // glDrawArrays(GL_TRIANGLES, 0, 6);
}

void RageDisplay_New::flipflopRenderDeInit()
{
	//glDeleteBuffers(1, &mFlipflopRenderVBO);
	//glDeleteVertexArrays(1, &mFlipflopRenderVAO);
}

void RageDisplay_New::EndFrame()
{
	GLDebugGroup g("EndFrame");

	flipflopRender();

	FrameLimitBeforeVsync(mWindow->GetActualVideoModeParams().rate);
	mWindow->SwapBuffers();
	FrameLimitAfterVsync();

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
		// Hey ITGaz here, I'm one of 'those people' mentioned above.
		// glFinish is terrible, and should never be called in any circumstances >:3
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
			// Wait up to 33ms for the fence - If we can't maintain 30fps then the
			// visual sync won't matter much to the player..
			glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, 33000);
		}
	}

	mWindow->Update();

	ProcessStatsOnFlip();

	static int i = 0;
	if (++i == 1000) {
		LOG->Info("FPS: %d", GetFPS());
		i = 0;
	}
}

uintptr_t RageDisplay_New::CreateTexture(
	RagePixelFormat fmt,
	RageSurface* img,
	bool generateMipMaps
)
{
	if (!img)
	{
		return 0;
	}

	if (fmt == RagePixelFormat::RagePixelFormat_PAL)
	{
		// TODO: Un-palletise if needed, like old renderer
		// TODO: Or directly upload palletted texture, but are those legacy?
		// TODO: This is a hot path
		return 0;
	}

	auto& texFormat = ragePixelFormatToGLFormat[fmt];

	GLuint tex = 0;
	glGenTextures(1, &tex);

	// TODO: There's method to this madness, I think..
	glActiveTexture(GL_TEXTURE0 + mTextureUnitForTexUploads);
	glBindTexture(GL_TEXTURE_2D, tex);

	// TODO: Old GL renderer has 'g_pWind->GetActualVideoModeParams().bAnisotropicFiltering'

	SetTextureFiltering(static_cast<TextureUnit>(mTextureUnitForTexUploads), true);
	SetTextureWrapping(static_cast<TextureUnit>(mTextureUnitForTexUploads), false);

	// TODO: mipmaps
	// TODO: Decide whether textures will be npot or no. For now yes they are for similicity.

	glTexImage2D(GL_TEXTURE_2D, 0, texFormat.internalfmt,
		img->w, img->h, 0,
		texFormat.format, texFormat.type,
		img->pixels
	);

	mTextures.emplace(tex);

	return tex;
}

void RageDisplay_New::UpdateTexture(
	uintptr_t texHandle,
	RageSurface* img,
	int xoffset, int yoffset, int width, int height
)
{
	//#error
}

void RageDisplay_New::DeleteTexture(uintptr_t texHandle)
{
	mTextures.erase(texHandle);
	GLuint t = texHandle;
	glDeleteTextures(1, &t);
}

void RageDisplay_New::ClearAllTextures()
{
	GLDebugGroup g("ClearAllTextures");

	// This is called after each element is rendered
	// but other than unbinding texture units doesn't do anything.
	// Since there's no detriment to doing nothing here, leave
	// the textures bound, don't waste the effort

	// TODO: But, there's a chance we need no textures bound when
	//       rendering some elements..perhaps we also need uniforms
	//       to signal whether textures should be used

	for (auto i = 0; i < static_cast<int>(TextureUnit::NUM_TextureUnit); ++i)
	{
		SetTexture(static_cast<TextureUnit>(i), 0);
	}
}

void RageDisplay_New::SetTexture(TextureUnit unit, uintptr_t texture)
{
	GLDebugGroup g("SetTexture");

	glActiveTexture(GL_TEXTURE0 + unit);
	if (texture != 0)
	{
		glBindTexture(GL_TEXTURE_2D, texture);
		mTextureSettings[unit].enabled = true;
		// This looks silly, but let's be specific about things
		mTextureUnits[unit] = unit;
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		mTextureSettings[unit].enabled = false;
		mTextureUnits[unit] = unit;
	}
}

void RageDisplay_New::SetTextureMode(TextureUnit unit, TextureMode mode)
{
	GLDebugGroup g(std::string("SetTextureMode ") + std::to_string(unit) + std::string(" ") + std::to_string(mode));
	mTextureSettings[unit].envMode = mode;
}

void RageDisplay_New::SetTextureWrapping(TextureUnit unit, bool wrap)
{
	GLDebugGroup g("SetTextureWrapping");

	glActiveTexture(GL_TEXTURE0 + unit);
	if (wrap)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
}

int RageDisplay_New::GetMaxTextureSize() const
{
	GLint s = 0;
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &s);
	return s;
}

void RageDisplay_New::SetTextureFiltering(TextureUnit unit, bool filter)
{
	GLDebugGroup g("SetTextureFiltering");

	glActiveTexture(GL_TEXTURE0 + unit);

	if (filter)
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// TODO: This is stupid and copied from old renderer - We should
		  // know whether a given texture has mipmaps without needing to
		  // talk to the GPU!
		GLint width0 = 0;
		GLint width1 = 0;
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width0);
		glGetTexLevelParameteriv(GL_TEXTURE_2D, 1, GL_TEXTURE_WIDTH, &width1);
		if (width1 != 0 && width0 > width1)
		{
			// Mipmaps are present for this texture
			if (mWindow->GetActualVideoModeParams().bTrilinearFiltering)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			}
			else
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			}
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		}
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	}
}

void RageDisplay_New::SetSphereEnvironmentMapping(TextureUnit tu, bool enabled)
{
	// TODO: Used for Model::DrawPrimitives
	// Might be tricky to implement, but see docs for glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP)
	// Probably translates to a fragment shader setting, that does the equivalent texture coord generation
	// TODO: Seems to activate in gameplay with some noteskins!?
	// LOG->Info("TODO: SetSphereEnvironmentMapping not implemented");
}

bool RageDisplay_New::IsZTestEnabled() const
{
	return mZTestMode != ZTestMode::ZTestMode_Invalid &&
		mZTestMode != ZTestMode::ZTEST_OFF;
}

bool RageDisplay_New::IsZWriteEnabled() const
{
	return mZWriteEnabled;
}

void RageDisplay_New::SetZWrite(bool enabled)
{
	GLDebugGroup g("SetZWrite");

	// if (!mZWriteEnabled && enabled)
	if (enabled)
	{
		mZWriteEnabled = true;
		glDepthMask(true);
	}
	// else if (mZWriteEnabled && !enabled)
	else
	{
		mZWriteEnabled = false;
		glDepthMask(false);
	}
}

void RageDisplay_New::SetZTestMode(ZTestMode mode)
{
	GLDebugGroup g("SetZTestMode");

	// TODO!!!
	// Previously we avoided this state change, but that caused
	// all depth operations to be broken...why!?
	/*if (mode == mZTestMode)
	{
		return;
	}*/

	switch (mode)
	{
	case ZTestMode::ZTEST_WRITE_ON_FAIL:
		glDepthFunc(GL_GREATER);
		break;
	case ZTestMode::ZTEST_WRITE_ON_PASS:
		glDepthFunc(GL_LEQUAL);
		break;
	case ZTestMode::ZTEST_OFF:
		glDepthFunc(GL_ALWAYS);
		break;
	default:
		FAIL_M(ssprintf("Invalid ZTestMode: %i", mode));
		glDepthFunc(GL_ALWAYS);
		break;
	}
}

void RageDisplay_New::SetZBias(float bias)
{
	GLDebugGroup g("SetZBias");

	float fNear = SCALE(bias, 0.0f, 1.0f, 0.05f, 0.0f);
	float fFar = SCALE(bias, 0.0f, 1.0f, 1.0f, 0.95f);
	// if (fNear != mFNear || fFar != mFFar)
	{
		mFNear = fNear;
		mFFar = fFar;
		glDepthRange(mFNear, mFFar);
	}
}

void RageDisplay_New::ClearZBuffer()
{
	GLDebugGroup g("ClearZBuffer");

	bool enabled = IsZWriteEnabled();
	if (!enabled)
	{
		SetZWrite(true);
	}
	glClear(GL_DEPTH_BUFFER_BIT);
	if (!enabled)
	{
		SetZWrite(false);
	}
}

void RageDisplay_New::SetBlendMode(BlendMode mode)
{
	GLDebugGroup g(std::string("SetBlendMode ") + std::to_string(mode));

	// TODO: Direct copy from old, but probably fine
	glEnable(GL_BLEND);

	if (glBlendEquation != nullptr)
	{
		if (mode == BLEND_INVERT_DEST)
			glBlendEquation(GL_FUNC_SUBTRACT);
		else if (mode == BLEND_SUBTRACT)
			glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
		else
			glBlendEquation(GL_FUNC_ADD);
	}

	int iSourceRGB, iDestRGB;
	int iSourceAlpha = GL_ONE, iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
	switch (mode)
	{
	case BLEND_NORMAL:
		iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case BLEND_ADD:
		iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE;
		break;
	case BLEND_SUBTRACT:
		iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case BLEND_MODULATE:
		iSourceRGB = GL_ZERO; iDestRGB = GL_SRC_COLOR;
		break;
	case BLEND_COPY_SRC:
		iSourceRGB = GL_ONE; iDestRGB = GL_ZERO;
		iSourceAlpha = GL_ONE; iDestAlpha = GL_ZERO;
		break;
	case BLEND_ALPHA_MASK:
		iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
		iSourceAlpha = GL_ZERO; iDestAlpha = GL_SRC_ALPHA;
		break;
	case BLEND_ALPHA_KNOCK_OUT:
		iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
		iSourceAlpha = GL_ZERO; iDestAlpha = GL_ONE_MINUS_SRC_ALPHA;
		break;
	case BLEND_ALPHA_MULTIPLY:
		iSourceRGB = GL_SRC_ALPHA; iDestRGB = GL_ZERO;
		break;
	case BLEND_WEIGHTED_MULTIPLY:
		/* output = 2*(dst*src).  0.5,0.5,0.5 is identity; darker colors darken the image,
		 * and brighter colors lighten the image. */
		iSourceRGB = GL_DST_COLOR; iDestRGB = GL_SRC_COLOR;
		break;
	case BLEND_INVERT_DEST:
		/* out = src - dst.  The source color should almost always be #FFFFFF, to make it "1 - dst". */
		iSourceRGB = GL_ONE; iDestRGB = GL_ONE;
		break;
	case BLEND_NO_EFFECT:
		iSourceRGB = GL_ZERO; iDestRGB = GL_ONE;
		iSourceAlpha = GL_ZERO; iDestAlpha = GL_ONE;
		break;
		DEFAULT_FAIL(mode);
	}

	if (glBlendFuncSeparateEXT)
		glBlendFuncSeparateEXT(iSourceRGB, iDestRGB, iSourceAlpha, iDestAlpha);
	else
		glBlendFunc(iSourceRGB, iDestRGB);
}

void RageDisplay_New::SetCullMode(CullMode mode)
{
	GLDebugGroup g("SetCullMode");

	switch (mode)
	{
	case CullMode::CULL_FRONT:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
		break;
	case CullMode::CULL_BACK:
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
		break;
	default:
		glDisable(GL_CULL_FACE);
	}
}

void RageDisplay_New::SetAlphaTest(bool enable)
{
	GLDebugGroup g(std::string("SetAlphaTest ") + std::to_string(enable));
	mMatrices.enableAlphaTest = enable;
	mMatrices.alphaTestThreshold = 1.0f / 256.0f;
}

void RageDisplay_New::SetMaterial(
	const RageColor& emissive,
	const RageColor& ambient,
	const RageColor& diffuse,
	const RageColor& specular,
	float shininess
)
{
	GLDebugGroup g("SetMaterial");

	// There is 1 material, fixed-function style
	// Behaviour ported from RageDisplay_Legacy
	// with roughly equivalent shading

	// Note from RageDisplay_Legacy:
	// "TRICKY:  If lighting is off, then setting the material 
	// will have no effect.  Even if lighting is off, we still
	// want Models to have basic color and transparency.
	// We can do this fake lighting by setting the vertex color."
	//
	// Here the logic is passed to shader, evaluated at draw time.

	mMaterial.emissive.x = emissive.r;
	mMaterial.emissive.y = emissive.g;
	mMaterial.emissive.z = emissive.b;
	mMaterial.emissive.w = emissive.a;

	mMaterial.ambient.x = ambient.r;
	mMaterial.ambient.y = ambient.g;
	mMaterial.ambient.z = ambient.b;
	mMaterial.ambient.w = ambient.a;

	mMaterial.diffuse.x = diffuse.r;
	mMaterial.diffuse.y = diffuse.g;
	mMaterial.diffuse.z = diffuse.b;
	mMaterial.diffuse.w = diffuse.a;

	mMaterial.specular.x = specular.r;
	mMaterial.specular.y = specular.g;
	mMaterial.specular.z = specular.b;
	mMaterial.specular.w = specular.a;

	mMaterial.shininess = shininess;
}

void RageDisplay_New::SetLighting(bool enable)
{
	mMatrices.enableLighting = enable;
}

void RageDisplay_New::SetLightOff(int index)
{
	mLights[index].enabled = false;
}

void RageDisplay_New::SetLightDirectional(
	int index,
	const RageColor& ambient,
	const RageColor& diffuse,
	const RageColor& specular,
	const RageVector3& dir)
{
	auto& l = mLights[index];
	l.enabled = true;
	l.ambient.x = ambient.r;
	l.ambient.y = ambient.g;
	l.ambient.z = ambient.b;
	l.ambient.w = ambient.a;

  l.diffuse.x = diffuse.r;
  l.diffuse.y = diffuse.g;
  l.diffuse.z = diffuse.b;
  l.diffuse.w = diffuse.a;

  l.specular.x = specular.r;
  l.specular.y = specular.g;
  l.specular.z = specular.b;
  l.specular.w = specular.a;
	// TODO: There's a chance that 'dir' is actually a position
	//       from some hardcoded (0, 0, 1) values, or from theme
	//       metrics. See THEME->GetMetricF("DancingCamera","LightX").
	//       For now mimic RageDisplay_Legacy - Pass through as if
	//       it's a direction, and maybe have broken lighting.
	//       Light settings may not be sensible in the theme
	//       anyway.
	// Lighting positions are in world space. This appears to be the same
	// as the old renderer.
	l.position = RageVector4(dir.x, dir.y, dir.z, 0.0f);
}

void RageDisplay_New::SetEffectMode(EffectMode effect)
{
	GLDebugGroup g(std::string("SetEffectMode ") + std::to_string(effect));

	auto shaderName = effectModeToShaderName(effect);
	if (!UseProgram(shaderName))
	{
		LOG->Info("SetEffectMode: Shader not available for mode: %i", effect);
		UseProgram(ShaderName::MegaShader);
	}
}

bool RageDisplay_New::IsEffectModeSupported(EffectMode effect)
{
	auto shaderName = effectModeToShaderName(effect);
	auto it = mShaderPrograms.find(shaderName);
	return it != mShaderPrograms.end();
}

void RageDisplay_New::SetCelShaded(int stage)
{
	GLDebugGroup g("SetCelShaded " + std::to_string(stage));
	// This function looks strange, and is for some reason
	// separate from SetEffectMode, but it's just selecting
	// cell shading in either vertex or fragment.
	if (stage == 1)
	{
		UseProgram(ShaderName::Shell);
	}
	else
	{
		UseProgram(ShaderName::Cel);
	}
}

RageDisplay_New::ShaderName RageDisplay_New::effectModeToShaderName(EffectMode effect)
{
	switch (effect)
	{
	case EffectMode_Normal:
		return ShaderName::MegaShader;
	case EffectMode_Unpremultiply:
		return ShaderName::Unpremultiply;
	case EffectMode_ColorBurn:
		return ShaderName::ColourBurn;
	case EffectMode_ColorDodge:
		return ShaderName::ColourDodge;
	case EffectMode_VividLight:
		return ShaderName::VividLight;
	case EffectMode_HardMix:
		return ShaderName::HardMix;
	case EffectMode_Overlay:
		return ShaderName::Overlay;
	case EffectMode_Screen:
		return ShaderName::Screen;
	case EffectMode_YUYV422:
		return ShaderName::TextureMatrixScaling;
	case EffectMode_DistanceField:
		return ShaderName::DistanceField;
	default:
		return ShaderName::MegaShader;
	}
}

RageCompiledGeometry* RageDisplay_New::CreateCompiledGeometry()
{
	auto g = new RageCompiledGeometryNew();
	mCompiledGeometry.insert(g);
	return g;
}

void RageDisplay_New::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
	auto g = dynamic_cast<RageCompiledGeometryNew*>(p);
	if (g)
	{
		mCompiledGeometry.erase(g);
		delete g;
	}
}

// Very much the hot path - draw quads is used for most actors, anything remotely sprite-like
// Test case: Everything
void RageDisplay_New::DrawQuadsInternal(const RageSpriteVertex v[], int numVerts)
{
	if (numVerts < 4)
	{
		return;
	}
	GLDebugGroup g("DrawQuadsInternal");

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);

	// Okay so Stepmania was written back when quads existed,
	// we have to convert to triangles now.
	// Geometry appears to be wound CCW
	std::vector<GLuint> elements;
	for (auto i = 0; i < numVerts; i += 4)
	{
		elements.emplace_back(i + 0);
		elements.emplace_back(i + 1);
		elements.emplace_back(i + 2);
		elements.emplace_back(i + 2);
		elements.emplace_back(i + 3);
		elements.emplace_back(i + 0);
	}
	GLuint ibo = 0;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint), elements.data(), GL_STATIC_DRAW);

  RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	glDrawElements(
		GL_TRIANGLES,
		elements.size(),
		GL_UNSIGNED_INT,
		nullptr
	);
	flipflopFBOs();

	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

// Very similar to draw quads but used less
// Test case: Density graph in simply love (Note: Intentionally includes invalid quads to produce graph spikes)
void RageDisplay_New::DrawQuadStripInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawQuadStripInternal");

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);

	// 0123 -> 102 123
	// 2345 -> 324 345
	std::vector<GLuint> elements;
	for (auto i = 0; i < numVerts - 2; i += 2)
	{
		elements.emplace_back(i + 1);
		elements.emplace_back(i + 0);
		elements.emplace_back(i + 2);
		elements.emplace_back(i + 1);
		elements.emplace_back(i + 2);
		elements.emplace_back(i + 3);
	}
	GLuint ibo = 0;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint), elements.data(), GL_STATIC_DRAW);

	RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	glDrawElements(
		GL_TRIANGLES,
		elements.size(),
		GL_UNSIGNED_INT,
		nullptr
	);
	flipflopFBOs();

	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

// Test case: Results screen stats (timeline) in simply love
void RageDisplay_New::DrawFanInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawFanInternal");

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);
	RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	glDrawArrays(GL_TRIANGLE_FAN, 0, numVerts);
	flipflopFBOs();

	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

// Test case: Unknown
void RageDisplay_New::DrawStripInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawStripInternal");

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);
	RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	glDrawArrays(GL_TRIANGLE_STRIP, 0, numVerts);
	flipflopFBOs();

	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

// Test case: Unknown
void RageDisplay_New::DrawTrianglesInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawTrianglesInternal");

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);
	RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	glDrawArrays(GL_TRIANGLES, 0, numVerts);
	flipflopFBOs();

	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

// Test case: Any 3D noteskin
void RageDisplay_New::DrawCompiledGeometryInternal(const RageCompiledGeometry* p, int meshIndex)
{
	GLDebugGroup g("DrawCompiledGeometryInternal");

	if (auto geom = dynamic_cast<const RageCompiledGeometryNew*>(p))
	{
		/*auto previousProg = mActiveShaderProgram.first;
    UseProgram(ShaderName::MegaShaderCompiledGeometry);*/

		UseProgram(ShaderName::MegaShader);

		// TODO: Bit ugly having it here, but compiled geometry doesn't _have_ per-vertex colour
		//       Temporarily switch over to the params needed for compiled geometry..
	  mMatrices.enableVertexColour = false;
	  mMatrices.enableTextureMatrixScale = geom->needsTextureMatrixScale(meshIndex);

		SetShaderUniforms();
		geom->Draw(meshIndex);

		mMatrices.enableVertexColour = true;
		mMatrices.enableTextureMatrixScale = false;

		// UseProgram(previousProg);
	}	
}

// Test case: Gameplay - Life bar line on graph, in simply love step statistics panel
// TODO: This one is weird, and needs a rewrite for modern GL -> We need to render line & points sure, but we do that differently these days
// TODO: Don't think smoothlines works, or otherwise these lines look worse than the old GL renderer
// Thanks RageDisplay_Legacy! - Carried lots of the hacks over for now
void RageDisplay_New::DrawLineStripInternal(const RageSpriteVertex v[], int numVerts, float lineWidth)
{
	GLDebugGroup g("DrawLineStripInternal");

	if (GetActualVideoModeParams().bSmoothLines)
	{
		/* Fall back on the generic polygon-based line strip. */
		RageDisplay::DrawLineStripInternal(v, numVerts, lineWidth);
		return;
	}

	/* fLineWidth is in units relative to object space, but OpenGL line and point sizes
	 * are in raster units (actual pixels).  Scale the line width by the average ratio;
	 * if object space is 640x480, and we have a 1280x960 window, we'll double the
	 * width. */
	{
		const RageMatrix* pMat = GetProjectionTop();
		float fW = 2 / pMat->m[0][0];
		float fH = -2 / pMat->m[1][1];
		float fWidthVal = float(GetActualVideoModeParams().width) / fW;
		float fHeightVal = float(GetActualVideoModeParams().height) / fH;
		lineWidth *= (fWidthVal + fHeightVal) / 2;
	}

	/* Clamp the width to the hardware max for both lines and points (whichever
	 * is more restrictive). */
	lineWidth = clamp(lineWidth, mLineWidthRange[0], mLineWidthRange[1]);
	lineWidth = clamp(lineWidth, mPointSizeRange[0], mPointSizeRange[1]);

	/* Hmm.  The granularity of lines and points might be different; for example,
	 * if lines are .5 and points are .25, we might want to snap the width to the
	 * nearest .5, so the hardware doesn't snap them to different sizes.  Does it
	 * matter? */
	glLineWidth(lineWidth);

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);
	RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	/* Draw a nice AA'd line loop.  One problem with this is that point and line
	 * sizes don't always precisely match, which doesn't look quite right.
	 * It's worth it for the AA, though. */
	glEnable(GL_LINE_SMOOTH);
	glDrawArrays(GL_LINE_STRIP, 0, numVerts);
	glDisable(GL_LINE_SMOOTH);

	/* Round off the corners.  This isn't perfect; the point is sometimes a little
	 * larger than the line, causing a small bump on the edge.  Not sure how to fix
	 * that. */
	glPointSize(lineWidth);

	/* Hack: if the points will all be the same, we don't want to draw
	 * any points at all, since there's nothing to connect.  That'll happen
	 * if both scale factors in the matrix are ~0.  (Actually, I think
	 * it's true if two of the three scale factors are ~0, but we don't
	 * use this for anything 3d at the moment anyway ...)  This is needed
	 * because points aren't scaled like regular polys--a zero-size point
	 * will still be drawn. */
	RageMatrix modelView;
	RageMatrixMultiply(&modelView, GetViewTop(), GetWorldTop());

	if (modelView.m[0][0] >= 1e-5 && modelView.m[1][1] >= 1e-5)
	{
		glEnable(GL_POINT_SMOOTH);
		glDrawArrays(GL_POINTS, 0, numVerts);
		glDisable(GL_POINT_SMOOTH);
	}

	flipflopFBOs();

	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

// Test case: Unknown
void RageDisplay_New::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawSymmetricQuadStripInternal");

	// TODO: This is obviously terrible, but lets just get things going
	GLuint vao = 0;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	// RageVColor is bgra
	std::vector<RageSpriteVertex> fixedVerts;
	for (auto i = 0; i < numVerts; ++i)
	{
		RageSpriteVertex vert = v[i];
		std::swap(vert.c.b, vert.c.r);
		fixedVerts.push_back(vert);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), fixedVerts.data(), GL_STATIC_DRAW);

	// Thanks RageDisplay_Legacy - No time to work this out,
	// ported index buffer logic directly
	int numPieces = (numVerts - 3) / 3;
	int numTriangles = numPieces * 4;
	std::vector<GLuint> elements;
	for (auto i = 0; i < numPieces; ++i)
	{
		// { 1, 3, 0 } { 1, 4, 3 } { 1, 5, 4 } { 1, 2, 5 }
		elements.emplace_back(i * 3 + 1);
		elements.emplace_back(i * 3 + 3);
		elements.emplace_back(i * 3 + 0);
		elements.emplace_back(i * 3 + 1);
		elements.emplace_back(i * 3 + 4);
		elements.emplace_back(i * 3 + 3);
		elements.emplace_back(i * 3 + 1);
		elements.emplace_back(i * 3 + 5);
		elements.emplace_back(i * 3 + 4);
		elements.emplace_back(i * 3 + 1);
		elements.emplace_back(i * 3 + 2);
		elements.emplace_back(i * 3 + 5);
	}

	GLuint ibo = 0;
	glGenBuffers(1, &ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elements.size() * sizeof(GLuint), elements.data(), GL_STATIC_DRAW);

	RageDisplay_New_ShaderProgram::configureVertexAttributesForSpriteRender();
	UseProgram(ShaderName::MegaShader);
	SetShaderUniforms();

	glDrawElements(
		GL_TRIANGLES,
		elements.size(),
		GL_UNSIGNED_INT,
		nullptr
	);
	flipflopFBOs();

	glDeleteBuffers(1, &ibo);
	glDeleteBuffers(1, &vbo);
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &vao);
}

/*
 * Copyright (c) 2023 Gareth Francis
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
