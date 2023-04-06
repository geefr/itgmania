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

#include <GL/glew.h>
#ifdef WINNT
# include <GL/wglew.h>
#endif
#include <RageFile.h>

namespace {
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

	class RageCompiledGeometryNew : public RageCompiledGeometry
	{
	public:

		void Allocate(const std::vector<msMesh>&) {}
		void Change(const std::vector<msMesh>&) {}
		void Draw(int iMeshIndex) const {}
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

  LoadShaderPrograms();

  // TODO
  // - MSAA
  // - Stencil buffer is enabled for some reason?

	return {};
}

RageDisplay_New::~RageDisplay_New()
{
  ClearAllTextures();
  for (auto& shader : mShaderPrograms)
  {
	  glDeleteProgram(shader.second);
  }
	if (mWindow)
	{
		delete mWindow;
	}
}

void RageDisplay_New::LoadShaderPrograms()
{
  LoadShaderProgram(ShaderName::RenderPlaceholder,
             "Data/Shaders/GLSL_400/renderplaceholder.vert",
	           "Data/Shaders/GLSL_400/renderplaceholder.frag");
}

void RageDisplay_New::LoadShaderProgram(ShaderName name, std::string vert, std::string frag)
{
  auto vertShader = LoadShader(GL_VERTEX_SHADER, vert);
  auto fragShader = LoadShader(GL_FRAGMENT_SHADER, frag);
  auto shaderProgram = glCreateProgram();
  glAttachShader(shaderProgram, vertShader);
  glAttachShader(shaderProgram, fragShader);
  glLinkProgram(shaderProgram);

  GLint success = 0;
  glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
  if (!success)
  {
    GLint logLength = 0;
    glGetProgramiv(shaderProgram, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(logLength, '\0');
		glGetProgramInfoLog(shaderProgram, logLength, nullptr, log.data());
	  ASSERT_M(success, (std::string("Failed to link shader program: ") + log).c_str());
  }

  mShaderPrograms[name] = shaderProgram;
}

GLuint RageDisplay_New::LoadShader(GLenum type, std::string source)
{
	RString buf;
	{
		RageFile file;
		if (!file.Open(source))
		{
			LOG->Warn("Error compiling shader %s: %s", source.c_str(), file.GetError().c_str());
			return 0;
		}

		if (file.Read(buf, file.GetFileSize()) == -1)
		{
			LOG->Warn("Error compiling shader %s: %s", source.c_str(), file.GetError().c_str());
			return 0;
		}
	}

  auto shader = glCreateShader(type);
  const char* cStr = buf.c_str();
  glShaderSource(shader, 1, &cStr, nullptr);
  glCompileShader(shader);

  GLint success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  ASSERT_M(success, "Failed to compile shader");

  return shader;
}

void RageDisplay_New::UseProgram(ShaderName name)
{
	auto it = mShaderPrograms.find(name);
	if (it == mShaderPrograms.end())
	{
		LOG->Warn("Invalid shader program requested: %i", name);
		return;
	}
	glUseProgram(it->second);
	mActiveShaderProgram = it->second;
}

void RageDisplay_New::InitVertexAttribsSpriteVertex()
{
  /*
	RageVector3 p; // position
	RageVector3 n; // normal
	RageVColor  c; // diffuse color
	RageVector2 t; // texture coordinates
	*/
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, p)));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, n)));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 4, GL_BYTE, GL_TRUE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, c)));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(RageSpriteVertex), reinterpret_cast<const void*>(offsetof(RageSpriteVertex, t)));
	glEnableVertexAttribArray(3);
}

void RageDisplay_New::SetShaderUniforms()
{
	glUniform1i(glGetUniformLocation(mActiveShaderProgram, "tex0"), 0);
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

  mVideoParams = p;

	auto err = mWindow->TryVideoMode(p, newDeviceCreated);
	if (!err.empty())
	{
		return err;
	}

	// TODO: Re-init caches, handle resolution change

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

/*
RageMatrix RageDisplay_New::GetOrthoMatrix(float l, float r, float b, float t, float zn, float zf)
{
  // TODO: If we're not OpenGL this needs to change
	RageMatrix m(
		2 / (r - l), 0, 0, 0,
		0, 2 / (t - b), 0, 0,
		0, 0, -2 / (zf - zn), 0,
		-(r + l) / (r - l), -(t + b) / (t - b), -(zf + zn) / (zf - zn), 1);
	return m;
}
*/

bool RageDisplay_New::BeginFrame()
{
  glClearColor(0.7f, 0.2f, 0.7f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  return true;
}

void RageDisplay_New::EndFrame()
{
	FrameLimitBeforeVsync(mWindow->GetActualVideoModeParams().rate);
	mWindow->SwapBuffers();
	FrameLimitAfterVsync();

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
  }

  auto& texFormat = ragePixelFormatToGLFormat[fmt];

  GLuint tex = 0;
  glGenTextures(1, &tex);

  // TODO: There's method to this madness, I think..
  TextureUnit texUnit = static_cast<TextureUnit>(mNumTextureUnitsCombined - 1);
  glActiveTexture(GL_TEXTURE0 + texUnit);
  glBindTexture(GL_TEXTURE_2D, tex);

  // TODO: Old GL renderer has 'g_pWind->GetActualVideoModeParams().bAnisotropicFiltering'

  SetTextureFiltering( texUnit, true );
  SetTextureWrapping( texUnit, false );

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
  glDeleteShader(texHandle);
}

void RageDisplay_New::ClearAllTextures()
{
  // This is called after each element is rendered
  // but other than unbinding texture units doesn't do anything.
  // Since there's no detriment to doing nothing here, leave
  // the textures bound, don't waste the effort
}

void RageDisplay_New::SetTexture(TextureUnit unit, uintptr_t texture)
{
  glActiveTexture(GL_TEXTURE0 + unit);
  glBindTexture(GL_TEXTURE_2D, texture);
}

void RageDisplay_New::SetTextureMode(TextureUnit unit, TextureMode mode)
{
  // TODO: You need to understand what this is doing, and whether
  //       it's actually used anywhere
  //
  // TODO: Reasonably sure the old version is using fixed function stuff
  //       so this would be done in-shader now?
}

void RageDisplay_New::SetTextureWrapping(TextureUnit unit, bool wrap)
{
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
		  if (mVideoParams.bTrilinearFiltering)
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
	if (!mZWriteEnabled && enabled)
	{
	  mZWriteEnabled = true;
	  glDepthMask(true);
  }
	else if (mZWriteEnabled && !enabled)
	{
		mZWriteEnabled = false;
		glDepthMask(false);
	}
}

void RageDisplay_New::SetZTestMode(ZTestMode mode)
{
	if (mode == mZTestMode)
	{
		return;
  }
	switch (mode)
	{
	  case ZTestMode::ZTEST_WRITE_ON_FAIL:
	    glEnable(GL_DEPTH_TEST);
		  glDepthFunc(GL_LEQUAL);
		  break;
	  case ZTestMode::ZTEST_WRITE_ON_PASS:
		  glEnable(GL_DEPTH_TEST);
		  glDepthFunc(GL_GREATER);
		  break;
	  default:
		  FAIL_M(ssprintf("Invalid ZTestMode: %i", mode));
		case ZTestMode::ZTEST_OFF:
			glDisable(GL_DEPTH_TEST);
  }
}

void RageDisplay_New::SetZBias(float bias)
{
	float fNear = SCALE(bias, 0.0f, 1.0f, 0.05f, 0.0f);
	float fFar = SCALE(bias, 0.0f, 1.0f, 1.0f, 0.95f);
	if( fNear != mFNear || fFar != mFFar )
	{
	  mFNear = fNear;
	  mFFar = fFar;
	  glDepthRange(mFNear, mFFar);
	}
}

void RageDisplay_New::ClearZBuffer()
{
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
  // TODO: glAlphaFunc -> fragment shader
	// Previously this was 0.01, rather than 0x01.
	// glAlphaFunc(GL_GREATER, 0.00390625 /* 1/256 */);

	if (enable)
	{
		glEnable(GL_ALPHA_TEST);
	}
	else
	{
		glDisable(GL_ALPHA_TEST);
	}
}

RageCompiledGeometry* RageDisplay_New::CreateCompiledGeometry()
{
	return new RageCompiledGeometryNew;
}

void RageDisplay_New::DeleteCompiledGeometry(RageCompiledGeometry* p)
{
  auto g = dynamic_cast<RageCompiledGeometryNew*>(p);
  if (g)
  {
	  delete g;
  }
}

void RageDisplay_New::DrawQuadsInternal(const RageSpriteVertex v[], int numVerts)
{
	if (numVerts < 4)
	{
		return;
  }

  // TODO: This is obviously terrible, but lets just get things going
  GLuint vao = 0;
  glCreateVertexArrays(1, &vao);
  glBindVertexArray(vao);

  GLuint vbo = 0;
  glGenBuffers(1, &vbo);

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, numVerts * sizeof(RageSpriteVertex), v, GL_STATIC_DRAW);
  InitVertexAttribsSpriteVertex();

  // Okay so Stepmania was written back when quads existed,
  // we have to convert to triangles now.
  // Work on the assumption that quads are wound CCW,
  // otherwise we can't be sure of anything.
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

  UseProgram(ShaderName::RenderPlaceholder);

  glDrawElements(
	  GL_TRIANGLES,
	  elements.size(),
	  GL_UNSIGNED_INT,
	  nullptr
  );

  glDeleteBuffers(1, &ibo);
  glDeleteBuffers(1, &vbo);
  glBindVertexArray(0);
  glDeleteBuffers(1, &vao);
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
