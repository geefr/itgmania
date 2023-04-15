#include "global.h"

#include "RageDisplay.h"
#include "RageDisplay_GL4.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageTimer.h"
#include "RageMath.h"
#include "RageTypes.h"
#include "RageUtil.h"
#include "RageSurface.h"
#include "DisplaySpec.h"

#include "RageSurfaceUtils.h"
#include "RageSurface_Save_PNG.h"

#include "compiledgeometry.h"

#include <GL/glew.h>
#ifdef WINNT
# include <GL/wglew.h>
#endif
#include <RageFile.h>

#include <algorithm>
#include <RageTextureManager.h>

namespace RageDisplay_GL4
{

namespace {
	const bool enableGLDebugGroups = false;
	const bool periodicShaderReload = false;

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
	void GLDebugMessage(std::string msg)
	{
	
	}

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

	struct GLFormatForRagePixelFormat {
		GLenum internalfmt;
		GLenum format;
		GLenum type;
	};
	// Not const, because callers aren't const..
	static std::map<RagePixelFormat, GLFormatForRagePixelFormat> ragePixelFormatToGLFormat = {
		{ RagePixelFormat::RagePixelFormat_RGBA8, { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE}},
		{ RagePixelFormat::RagePixelFormat_BGRA8, { GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE}},
		{ RagePixelFormat::RagePixelFormat_RGBA4, { GL_RGBA8, GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4}},
	};
}

RageDisplay_GL4::FrameBuffer::FrameBuffer(GLuint width, GLuint height, GLint tempTexUnit)
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

RageDisplay_GL4::FrameBuffer::~FrameBuffer()
{
	glDeleteRenderbuffers(1, &mDepthRBO);
	glDeleteTextures(1, &mTex);
	glDeleteFramebuffers(1, &mFBO);
}

void RageDisplay_GL4::FrameBuffer::bindRenderTarget()
{
	glBindFramebuffer(GL_FRAMEBUFFER, mFBO);
}

void RageDisplay_GL4::FrameBuffer::unbindRenderTarget()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RageDisplay_GL4::FrameBuffer::bindTexture(GLint texUnit)
{
	glActiveTexture(GL_TEXTURE0 + texUnit);
	glBindTexture(GL_TEXTURE_2D, mTex);
}

RageDisplay_GL4::RageDisplay_GL4()
{
	LOG->MapLog("renderer", "Current renderer: new");
}

RString RageDisplay_GL4::Init(const VideoModeParams& p, bool /* bAllowUnacceleratedRenderer */)
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

	const float fGLUVersion = StringToFloat((const char*)gluGetString(GLU_VERSION));
	mGLUVersion = lrintf(fGLUVersion * 10);

	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &mNumTextureUnits);
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &mNumTextureUnitsCombined);

	glGetFloatv(GL_LINE_WIDTH_RANGE, mLineWidthRange);
	glGetFloatv(GL_POINT_SIZE_RANGE, mPointSizeRange);
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &mMaxTextureSize);

	mTextureUnitForTexUploads = mNumTextureUnitsCombined - 1;
	mTextureUnitForFBOUploads = mNumTextureUnitsCombined - 2;
	mTextureUnitForFlipFlopRender = mNumTextureUnitsCombined - 3;

	LoadShaderPrograms();

	flipflopRenderInit();

	mRenderer.init();

	// TODO
	// - MSAA
	//   - Either globally on context
	//   - Or since we have flipflop now, MSAA there and add a postprocessing shader >:D

	return {};
}

RageDisplay_GL4::~RageDisplay_GL4()
{
	flipflopRenderDeInit();
	ClearAllTextures();
	mShaderPrograms.clear();
	if (mWindow)
	{
		delete mWindow;
	}
}

void RageDisplay_GL4::ResolutionChanged()
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

bool RageDisplay_GL4::SupportsTextureFormat(RagePixelFormat fmt, bool realtime)
{
  // TODO: CreateTexture can't upload paletted textures, and we don't want to deal with
  //       them in any situation if it can be avoided
  // TODO: Also reject anything we don't explicitly support yet.
  //       Will need a lot of testing, but I'd rather have slow texture uploads than
  //       crashes or rendering bugs.
  // TODO: RGBA4 is required - otherwise asserts
  return ragePixelFormatToGLFormat.find(fmt) != ragePixelFormatToGLFormat.end();
}

bool RageDisplay_GL4::SupportsSurfaceFormat(RagePixelFormat fmt)
{
	// TODO: CreateTexture can't upload paletted textures, and we don't want to deal with
	//       them in any situation if it can be avoided
	// TODO: Also reject anything we don't explicitly support yet.
	//       Will need a lot of testing, but I'd rather have slow texture uploads than
	//       crashes or rendering bugs.
	// TODO: RGBA4 is required - otherwise asserts
	return ragePixelFormatToGLFormat.find(fmt) != ragePixelFormatToGLFormat.end();
}

void RageDisplay_GL4::LoadShaderPrograms(bool failOnError)
{
  mShaderPrograms.clear();

  // This shader supports both sprite and model rendering
  // But if it's loaded twice, it should reduce the number of state changes
  // assuming that most uniforms remain the same across multiple uses of
  // the shader.
  // TODO: Shader preprocessing could cut down the megashader somewhat..

  mShaderPrograms[ShaderName::MegaShader] = std::make_shared<ShaderProgram>(
    std::string("Data/Shaders/GLSL_400/megashader.vert"),
	  std::string("Data/Shaders/GLSL_400/megashader.frag")
	);

  // TODO: For some reason enabling the 2nd instance of this shader breaks everything.
  //       Perhaps that also happens for any other shader too - Some assumed state between them somehow?
  mShaderPrograms[ShaderName::MegaShaderCompiledGeometry] = std::make_shared<ShaderProgram>(
	  std::string("Data/Shaders/GLSL_400/megashader.vert"),
		  std::string("Data/Shaders/GLSL_400/megashader.frag")
  );

  for (auto& p : mShaderPrograms)
  {
	  auto success = p.second->init();
		ASSERT_M(!failOnError || success, "Failed to compile shader program");
  }

  // TODO: Doesn't work, but bring it back as a quad/preprocessing shader at some point
  // TODO: Will need a common base class for shader programs / interfaces..
	/*LoadShaderProgram(ShaderName::FlipFlopFinal,
		"Data/Shaders/GLSL_400/flipflopfinal.vert",
		"Data/Shaders/GLSL_400/flipflopfinal.frag",
		failOnError);*/
}

bool RageDisplay_GL4::UseProgram(ShaderName name)
{
  auto it = mShaderPrograms.find(name);
  if (it == mShaderPrograms.end())
  {
		LOG->Warn("Invalid shader program requested: %i", name);
		return false;
  }

  auto s = mRenderer.state();
  s.shaderProgram = it->second;
  mRenderer.setState(s);
  return true;
}

void RageDisplay_GL4::SetCurrentMatrices()
{
	auto s = mRenderer.state();
	// Matrices
	RageMatrixMultiply(&s.uniformBlockMatrices.projection, GetCentering(), GetProjectionTop());

	// TODO: Related to render to texture
	/*if (g_bInvertY)
	{
		RageMatrix flip;
		RageMatrixScale(&flip, +1, -1, +1);
		RageMatrixMultiply(&projection, &flip, &projection);
	}*/

	RageMatrixMultiply(&s.uniformBlockMatrices.modelView, GetViewTop(), GetWorldTop());
	s.uniformBlockMatrices.texture = *GetTextureTop();
	mRenderer.setState(s);
}

void RageDisplay_GL4::GetDisplaySpecs(DisplaySpecs& out) const
{
	out.clear();
	if (mWindow)
	{
		mWindow->GetDisplaySpecs(out);
	}
}

RString RageDisplay_GL4::TryVideoMode(const VideoModeParams& p, bool& newDeviceCreated)
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
			s.second->invalidate();
			s.second->init();
		}
	}

	ResolutionChanged();

	return {};
}

RageSurface* RageDisplay_GL4::CreateScreenshot()
{
	/*const RagePixelFormatDesc &desc = PIXEL_FORMAT_DESC[RagePixelFormat_RGB8];
	RageSurface *image = CreateSurface(
		640, 480, desc.bpp,
		desc.masks[0], desc.masks[1], desc.masks[2], desc.masks[3] );

	memset( image->pixels, 0, 480*image->pitch );

	return image;*/

	return nullptr;
}

const RageDisplay::RagePixelFormatDesc* RageDisplay_GL4::GetPixelFormatDesc(RagePixelFormat pf) const
{
	ASSERT(pf >= 0 && pf < NUM_RagePixelFormat);
	return &PIXEL_FORMAT_DESC[pf];
}

bool RageDisplay_GL4::BeginFrame()
{
	//// TODO: A hack to let me reload shaders without restarting the game
	//if (periodicShaderReload)
	//{
	//	static int i = 0;
	//	if (++i == 100)
	//	{
	//		auto previousShader = 
	//		LoadShaderPrograms(false);
	//		// ResolutionChanged();
	//		UseProgram(previousShader);

	//		i = 0;
	//	}
	//}

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

	auto s = mRenderer.state();
	s.globalState.viewPort = RageVector4(0, 0, width, height);
  mRenderer.setState(s);
	mRenderer.clear();

	mRenderer.flushCommandQueue();
	bool beginFrame = RageDisplay::BeginFrame();
	// TODO: Offscreen render target / FBOs
	/*if (beginFrame && UseOffscreenRenderTarget()) {
		offscreenRenderTarget->BeginRenderingTo(false);
	}*/

	return beginFrame;
}

void RageDisplay_GL4::flipflopFBOs()
{
	// mFlip.swap(mFlop);

	// mFlip->bindRenderTarget();
	// mFlop->bindTexture(mTextureUnitForFlipFlopRender);
}

void RageDisplay_GL4::flipflopRenderInit()
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

void RageDisplay_GL4::flipflopRender()
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

void RageDisplay_GL4::flipflopRenderDeInit()
{
	//glDeleteBuffers(1, &mFlipflopRenderVBO);
	//glDeleteVertexArrays(1, &mFlipflopRenderVAO);
}

void RageDisplay_GL4::EndFrame()
{
	GLDebugGroup g("EndFrame");

	mRenderer.flushCommandQueue();

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

	mWindow->Update();

	ProcessStatsOnFlip();
}

uintptr_t RageDisplay_GL4::CreateTexture(
	RagePixelFormat fmt,
	RageSurface* img,
	bool generateMipMaps
)
{
	if(enableGLDebugGroups)
	{
		std::string msg = "CreateTexture ";
		msg += std::to_string(fmt);
		msg += " ";
		msg += std::to_string(img->w);
		msg += " ";
		msg += std::to_string(img->h);
		GLDebugGroup g(msg);
	}

	if (!img)
	{
		return 0;
	}

	/**
	 * Palleted Textures
	 * -----------------
	 *
	 * If the surface claims to support palleted textures, RageBitmapTexture::create
	 * will change fmt to be RagePixelFormat_RGBA8 or similar - Not the original format of the image.
	 *
	 * Checking for a paletted image is done by pImg->format->BitsPerPixel == 8
	 * Unless it's greyscale, and then it goes through RageSurfaceUtils::PalettizeToGrayscale
	 * (Which if I read it correctly, generates an 8-bit image that's both paletted and contains greyscale info
	 *  in a custom format).
	 *
	 * TODO: For now i'm making DISPLAY->SupportsTextureFormat(RagePixelFormat_PAL) return false.
	 *       That should force an RGBA8 format, and un-palette the texture before calling this function.
	 *       But it doesn't, see below :(
	 */
	if( !SupportsTextureFormat(fmt) )
	{
		return 0;
	}

	auto& texFormat = ragePixelFormatToGLFormat[fmt];
//	LOG->Info("Trying to load texture: fmt = %i texFormat.internal = %i texFormat.format = %i texFormat.type = %i w = %i h = %i",
//		fmt, texFormat.internalfmt, texFormat.format, texFormat.type, img->w, img->h
//	);

	/*
		TODO: One would think that a surface marked as RGBA8 is actually that,
			due to the logic in RageSurfaceUtils::ConvertSurface (I think) palleted images
			can get through. Observed with the following format.
			If this happens the image actually has 1 byte per pixel, not 4, and passing
			it through to OpenGL will crash.
			These are passed in marked as RGBA8, so the fmt parameter to this function
			cannot be trusted.

		/Themes/_fallback/Graphics/StreamDisplay hot.png
		p *img
		$2 = {format = 0xb0bded8, fmt = {BytesPerPixel = 1, BitsPerPixel = 8, Mask = {_M_elems = {0, 0, 0, 0}}, Shift = {_M_elems = {0, 0, 0, 
				0}}, Loss = {_M_elems = {0, 0, 0, 0}}, Rmask = @0xb0bdee0, Gmask = @0xb0bdee4, Bmask = @0xb0bdee8, Amask = @0xb0bdeec, 
				Rshift = @0xb0bdef0, Gshift = @0xb0bdef4, Bshift = @0xb0bdef8, Ashift = @0xb0bdefc, 
				palette = std::unique_ptr<RageSurfacePalette> = {get() = 0xb7ff400}}, pixels = 0xbc8a670 "", pixels_owned = true, w = 128, h = 32, 
		Npitch = 128, flags = 0}
	*/
	if( img->fmt.palette )
	{
		// Copy the data over to an _actual_ RGBA8 image then try again
		// TODO: For some reason the component masks need to be backwards to get RGBA out
	  // TODO: I have no idea why this is, and it's possibly just for the images I've seen so far
		std::unique_ptr<RageSurface> tmpSurface( CreateSurface( img->w, img->h, 32, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000 ) );
		RageSurfaceUtils::CopySurface(img, tmpSurface.get());

		/*
		static int imgIndex = 0;
		RageFile f;
		if (f.Open((std::string("/Logs/RageDisplay_GL4_CreateTexture_Palleted_") + std::to_string(imgIndex++) + std::string(".png")).c_str(), RageFile::WRITE))
		{
			RString err;
			RageSurfaceUtils::SavePNG(tmpSurface.get(), f, err);
		}
		*/
		
		return CreateTexture(
			RagePixelFormat_RGBA8,
			tmpSurface.get(),
			generateMipMaps
		);
	}

	GLuint tex = 0;
	glGenTextures(1, &tex);

	// Dedicate one texture unit to uploads, to avoid altering
	// the state of any units used for render (0-4).
	// Note that we can't call over to SetTextureFiltering or similar here,
	// since the main draw functions are configured for deferred rendering.
	// Instead we ensure no state-tracked texture units are modified, that
	// the texture is configured as needed with opengl, and that the state
	// is updated to match the opengl state.
	// (If we didn't, the first time this texture is rendered, its filtering
	//  mode would be reset to GL_LINEAR_MIPMAP_LINEAR, and if it hadn't
	//  otherwise been updated through SetTextureFiltering it would then
	//  render incorrectly)
	auto s = mRenderer.state();
	s.addTexture(tex, generateMipMaps);
	glActiveTexture(GL_TEXTURE0 + mTextureUnitForTexUploads);
	glBindTexture(GL_TEXTURE_2D, tex);

	// TODO: Old GL renderer has 'g_pWind->GetActualVideoModeParams().bAnisotropicFiltering'
	//       In the new case, this would be more texture settings for anisotropy

	// SetTextureFiltering(static_cast<TextureUnit>(mTextureUnitForTexUploads), true);
	if (generateMipMaps)
	{
		if (mWindow->GetActualVideoModeParams().bTrilinearFiltering)
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			s.textureState[tex].minFilter = GL_LINEAR_MIPMAP_LINEAR;
			s.textureState[tex].magFilter = GL_LINEAR;
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			s.textureState[tex].minFilter = GL_LINEAR_MIPMAP_NEAREST;
			s.textureState[tex].magFilter = GL_LINEAR;
		}
	}
	else
	{
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		s.textureState[tex].minFilter = GL_LINEAR;
		s.textureState[tex].magFilter = GL_LINEAR;
	}

	// SetTextureWrapping(static_cast<TextureUnit>(mTextureUnitForTexUploads), false);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	s.textureState[tex].wrapMode = GL_CLAMP_TO_EDGE;

	glPixelStorei(GL_UNPACK_ROW_LENGTH, img->pitch / img->format->BytesPerPixel);
	glTexImage2D(GL_TEXTURE_2D, 0, texFormat.internalfmt,
		img->w, img->h, 0,
		texFormat.format, texFormat.type,
		img->pixels
	);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

	if (generateMipMaps) glGenerateMipmap(GL_TEXTURE_2D);

	glBindTexture(GL_TEXTURE_2D, 0);

	mRenderer.setState(s);
	return tex;
}

void RageDisplay_GL4::UpdateTexture(
	uintptr_t texHandle,
	RageSurface* img,
	int xoffset, int yoffset, int width, int height
)
{
	//#error

	// Tell the renderer about the texture, so it can state-track
	// TODO: When a texture is invalidated we need to flush batches
	//       or at least if the invalidated texture is referenced by
	//       any pending draw commands
  mRenderer.flushCommandQueue();
}

void RageDisplay_GL4::DeleteTexture(uintptr_t texHandle)
{
	// TODO: When a texture is invalidated we need to flush batches
  //       or at least if the invalidated texture is referenced by
  //       any pending draw commands
	mRenderer.flushCommandQueue();

	GLuint t = texHandle;
	glDeleteTextures(1, &t);

	// Tell the renderer about the new texture, so it can state-track
	auto s = mRenderer.state();
	s.removeTexture(texHandle);
	mRenderer.setState(s);
}

void RageDisplay_GL4::ClearAllTextures()
{
	GLDebugGroup g("ClearAllTextures");

	// This is called after each element is rendered
	// but other than unbinding texture units doesn't do anything.
	// Since there's no detriment to doing nothing here, leave
	// the textures bound, don't waste the effort

	// We do need to disable all the textures however, to make
	// the shaders act in a similar manner to the old GL renderer
	auto s = mRenderer.state();
	for( auto i = 0; i < ShaderProgram::MaxTextures; ++i )
	{
		s.bindTexture(GL_TEXTURE0 + i, 0);
	}
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetTexture(TextureUnit unit, uintptr_t texture)
{
	GLDebugGroup g("SetTexture");
	auto s = mRenderer.state();
	s.bindTexture(GL_TEXTURE0 + unit, texture);
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetTextureMode(TextureUnit unit, TextureMode mode)
{
	GLDebugGroup g(std::string("SetTextureMode ") + std::to_string(unit) + std::string(" ") + std::to_string(mode));
	auto s = mRenderer.state();
	s.uniformBlockTextureSettings[unit].envMode = mode;
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetTextureWrapping(TextureUnit unit, bool wrap)
{
	GLDebugGroup g("SetTextureWrapping");
	auto s = mRenderer.state();
	s.textureWrap(GL_TEXTURE0 + unit, wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetTextureFiltering(TextureUnit unit, bool filter)
{
	GLDebugGroup g("SetTextureFiltering");
	auto s = mRenderer.state();
	if (filter)
	{
		// Yuck, but it's better than the old version
		if( s.textureHasMipMaps(s.boundTexture(unit)) )
		{
			// Mipmaps are present for this texture
			if (mWindow->GetActualVideoModeParams().bTrilinearFiltering)
			{
				s.textureFilter(GL_TEXTURE0 + unit, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
			}
			else
			{
				s.textureFilter(GL_TEXTURE0 + unit, GL_LINEAR_MIPMAP_NEAREST, GL_LINEAR);
			}
		}
		else
		{
			s.textureFilter(GL_TEXTURE0 + unit, GL_LINEAR, GL_LINEAR);
		}
	}
	else
	{
		s.textureFilter(GL_TEXTURE0 + unit, GL_NEAREST, GL_NEAREST);
	}
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetSphereEnvironmentMapping(TextureUnit tu, bool enabled)
{
	// TODO: Used for Model::DrawPrimitives
	// Might be tricky to implement, but see docs for glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP)
	// Probably translates to a fragment shader setting, that does the equivalent texture coord generation
	// TODO: Seems to activate in gameplay with some noteskins!?
	// LOG->Info("TODO: SetSphereEnvironmentMapping not implemented");
}

bool RageDisplay_GL4::IsZTestEnabled() const
{
	// Yes
	// TODO: It's always on, but who is asking and why?
	return true;
}

bool RageDisplay_GL4::IsZWriteEnabled() const
{
	return mRenderer.state().globalState.depthWriteEnabled;
}

void RageDisplay_GL4::SetZWrite(bool enabled)
{
	GLDebugGroup g("SetZWrite");
	auto s = mRenderer.state();
	s.globalState.depthWriteEnabled = enabled;
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetZTestMode(ZTestMode mode)
{
	GLDebugGroup g("SetZTestMode");
	auto s = mRenderer.state();
	switch (mode)
	{
	case ZTestMode::ZTEST_WRITE_ON_FAIL:
		s.globalState.depthFunc = GL_GREATER;
		break;
	case ZTestMode::ZTEST_WRITE_ON_PASS:
		s.globalState.depthFunc = GL_LEQUAL;
		break;
	case ZTestMode::ZTEST_OFF:
		s.globalState.depthFunc = GL_ALWAYS;
		break;
	default:
		FAIL_M(ssprintf("Invalid ZTestMode: %i", mode));
		s.globalState.depthFunc = GL_ALWAYS;
		break;
	}
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetZBias(float bias)
{
	GLDebugGroup g("SetZBias");
	auto s = mRenderer.state();
	float fNear = SCALE(bias, 0.0f, 1.0f, 0.05f, 0.0f);
	float fFar = SCALE(bias, 0.0f, 1.0f, 1.0f, 0.95f);
	s.globalState.depthNear = fNear;
	s.globalState.depthFar = fFar;
	mRenderer.setState(s);
}

void RageDisplay_GL4::ClearZBuffer()
{
	GLDebugGroup g("ClearZBuffer");

	// TODO: This is called between EVERY NOTE WE RENDER
	//       Surely we can improve on this, but perhaps
	//       the fix needs to be outside RageDisplay
	mRenderer.clearDepthBuffer();
}

void RageDisplay_GL4::SetBlendMode(BlendMode mode)
{
	GLDebugGroup g(std::string("SetBlendMode ") + std::to_string(mode));

	GLenum blendEq = GL_FUNC_ADD;
	if (mode == BLEND_INVERT_DEST)
		blendEq = GL_FUNC_SUBTRACT;
	else if (mode == BLEND_SUBTRACT)
		blendEq = GL_FUNC_REVERSE_SUBTRACT;

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
	auto s = mRenderer.state();
	s.globalState.blendEq = blendEq;
	s.globalState.blendSourceRGB = iSourceRGB;
	s.globalState.blendDestRGB = iDestRGB;
	s.globalState.blendSourceAlpha = iSourceAlpha;
	s.globalState.blendDestAlpha = iDestAlpha;
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetCullMode(CullMode mode)
{
	GLDebugGroup g("SetCullMode");
	auto s = mRenderer.state();
	switch (mode)
	{
	case CullMode::CULL_FRONT:
		s.globalState.cullEnabled = true;
		s.globalState.cullFace = GL_FRONT;
		break;
	case CullMode::CULL_BACK:
		s.globalState.cullEnabled = true;
		s.globalState.cullFace = GL_BACK;
		break;
	default:
		s.globalState.cullEnabled = false;
	}
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetAlphaTest(bool enable)
{
	GLDebugGroup g(std::string("SetAlphaTest ") + std::to_string(enable));
	auto s = mRenderer.state();
	s.uniformBlockMatrices.enableAlphaTest = enable;
	if (enable)
	{
		s.uniformBlockMatrices.alphaTestThreshold = 1.0f / 256.0f;
  }
  mRenderer.setState(s);
}

void RageDisplay_GL4::SetMaterial(
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
	auto s = mRenderer.state();
	s.uniformBlockMaterial.emissive.x = emissive.r;
	s.uniformBlockMaterial.emissive.y = emissive.g;
	s.uniformBlockMaterial.emissive.z = emissive.b;
	s.uniformBlockMaterial.emissive.w = emissive.a;

	s.uniformBlockMaterial.ambient.x = ambient.r;
	s.uniformBlockMaterial.ambient.y = ambient.g;
	s.uniformBlockMaterial.ambient.z = ambient.b;
	s.uniformBlockMaterial.ambient.w = ambient.a;
 
	s.uniformBlockMaterial.diffuse.x = diffuse.r;
	s.uniformBlockMaterial.diffuse.y = diffuse.g;
	s.uniformBlockMaterial.diffuse.z = diffuse.b;
	s.uniformBlockMaterial.diffuse.w = diffuse.a;
 
	s.uniformBlockMaterial.specular.x = specular.r;
	s.uniformBlockMaterial.specular.y = specular.g;
	s.uniformBlockMaterial.specular.z = specular.b;
	s.uniformBlockMaterial.specular.w = specular.a;
 
	s.uniformBlockMaterial.shininess = shininess;
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetLighting(bool enable)
{
	auto s = mRenderer.state();
	s.uniformBlockMatrices.enableLighting = enable;
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetLightOff(int index)
{
	auto s = mRenderer.state();
	s.uniformBlockLights[index].enabled = false;
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetLightDirectional(
	int index,
	const RageColor& ambient,
	const RageColor& diffuse,
	const RageColor& specular,
	const RageVector3& dir)
{
	auto s = mRenderer.state();
	auto& l = s.uniformBlockLights[index];
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
	mRenderer.setState(s);
}

void RageDisplay_GL4::SetEffectMode(EffectMode effect)
{
	GLDebugGroup g(std::string("SetEffectMode ") + std::to_string(effect));

	auto shaderName = effectModeToShaderName(effect);
	if (!UseProgram(shaderName))
	{
		LOG->Info("SetEffectMode: Shader not available for mode: %i", effect);
		UseProgram(ShaderName::MegaShader);
	}
}

bool RageDisplay_GL4::IsEffectModeSupported(EffectMode effect)
{
	auto shaderName = effectModeToShaderName(effect);
	auto it = mShaderPrograms.find(shaderName);
	return it != mShaderPrograms.end();
}

void RageDisplay_GL4::SetCelShaded(int stage)
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

ShaderName RageDisplay_GL4::effectModeToShaderName(EffectMode effect)
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

RageCompiledGeometry* RageDisplay_GL4::CreateCompiledGeometry()
{
	auto g = new CompiledGeometry();
	mCompiledGeometry.insert(g);
	return g;
}

void RageDisplay_GL4::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
	auto g = dynamic_cast<CompiledGeometry*>(p);
	if (g)
	{
		mCompiledGeometry.erase(g);
		delete g;
	}
}

// Very much the hot path - draw quads is used for most actors, anything remotely sprite-like
// Test case: Everything
void RageDisplay_GL4::DrawQuadsInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawQuadsInternal");
	SetCurrentMatrices();
	mRenderer.drawQuads(v, numVerts);
}

// Very similar to draw quads but used less
// Test case: Density graph in simply love (Note: Intentionally includes invalid quads to produce graph spikes)
void RageDisplay_GL4::DrawQuadStripInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawQuadStripInternal");
	SetCurrentMatrices();
	mRenderer.drawQuadStrip(v, numVerts);
}

// Test case: Results screen stats (timeline) in simply love
void RageDisplay_GL4::DrawFanInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawFanInternal");
  SetCurrentMatrices();
  mRenderer.drawTriangleFan(v, numVerts);
}

// Test case: Unknown
void RageDisplay_GL4::DrawStripInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawStripInternal");
	SetCurrentMatrices();
	mRenderer.drawTriangleStrip(v, numVerts);
}

// Test case: Unknown
void RageDisplay_GL4::DrawTrianglesInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawTrianglesInternal");
	SetCurrentMatrices();
	mRenderer.drawTriangles(v, numVerts);
}

// Test case: Any 3D noteskin
void RageDisplay_GL4::DrawCompiledGeometryInternal(const RageCompiledGeometry* p, int meshIndex)
{
  // TODO: Because it's broken :)
  return;

	// Note that it's fine to do immediate renders
	// as long as the renderer is flushed, and
	// state is resynchronised before returning
	mRenderer.flushCommandQueue();
	SetCurrentMatrices();

	auto oldState = mRenderer.state();
	GLDebugGroup g("DrawCompiledGeometryInternal");

	if (auto geom = dynamic_cast<const CompiledGeometry*>(p))
	{
		// Note that for now compiled geometry uses the same shader as sprite rendering.
		// But because the shader is loaded twice, some uniforms never need to be changed
		// giving a small performance boost. Shader preprocessing would be better long term.
    UseProgram(ShaderName::MegaShaderCompiledGeometry);

		// TODO: Bit ugly having it here, but compiled geometry doesn't _have_ per-vertex colour
		//       Temporarily switch over to the params needed for compiled geometry..
		auto s = mRenderer.state();
		s.uniformBlockMatrices.enableVertexColour = false;
		s.uniformBlockMatrices.enableTextureMatrixScale = geom->needsTextureMatrixScale(meshIndex);
		s.updateGPUState(oldState);
		geom->Draw(meshIndex);
	}

	// Reset the gpu state (in full!)
	// to resync the renderer
	oldState.updateGPUState();
	mRenderer.setState(oldState);
}

// Test case: Gameplay - Life bar line on graph, in simply love step statistics panel
// TODO: This one is weird, and needs a rewrite for modern GL -> We need to render line & points sure, but we do that differently these days
// TODO: Don't think smoothlines works, or otherwise these lines look worse than the old GL renderer
// Thanks RageDisplay_Legacy! - Carried lots of the hacks over for now
void RageDisplay_GL4::DrawLineStripInternal(const RageSpriteVertex v[], int numVerts, float lineWidth)
{
	// Note that it's fine to do immediate renders
	// as long as the renderer is flushed, and
	// state is resynchronised before returning
	mRenderer.flushCommandQueue();
	auto oldState = mRenderer.state();
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

	auto s = mRenderer.state();
	/* Hmm.  The granularity of lines and points might be different; for example,
	 * if lines are .5 and points are .25, we might want to snap the width to the
	 * nearest .5, so the hardware doesn't snap them to different sizes.  Does it
	 * matter? */
	s.globalState.lineWidth = lineWidth;
	/* Draw a nice AA'd line loop.  One problem with this is that point and line
	 * sizes don't always precisely match, which doesn't look quite right.
	 * It's worth it for the AA, though. */
	s.globalState.lineSmoothEnabled = true;

	mRenderer.setState(s);
	SetCurrentMatrices();

	mRenderer.drawLinestrip(v, numVerts);
	s = mRenderer.state();
	s.globalState.lineSmoothEnabled = false;
	mRenderer.setState(s);

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
		/* Round off the corners.  This isn't perfect; the point is sometimes a little
		 * larger than the line, causing a small bump on the edge.  Not sure how to fix
		 * that. */
		s = mRenderer.state();
		s.globalState.pointSize = lineWidth;
		// TODO: Point smoothing isn't CORE
		// s.globalState.pointSmoothEnabled = true;
		mRenderer.setState(s);

		mRenderer.drawPoints(v, numVerts);

		// TODO: If this is the only thing rendering points, and we
		//       always enable smoothing, can we just turn that on
		//       by default?
		// s = mRenderer.state();
		// s.globalState.pointSmoothEnabled = false;
		// mRenderer.setState(s);
	}
}

// Test case: Unknown
void RageDisplay_GL4::DrawSymmetricQuadStripInternal(const RageSpriteVertex v[], int numVerts)
{
	GLDebugGroup g("DrawSymmetricQuadStripInternal");
	SetCurrentMatrices();
	mRenderer.drawSymmetricQuadStrip(v, numVerts);
}

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
