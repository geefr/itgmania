/* RageDisplay_New

This is a likely misguided effort to rewrite the renderer with _some_ modern graphics tech.

Initially this is based on RageDisplay_Legacy (OpenGL), but isn't guaranteed to use GL going forward

I will probably fail - Gaz
*/

#pragma once

#include <arch/LowLevelWindow/LowLevelWindow.h>
#include "RageDisplay_New_Geometry.h"

#include <memory>
#include <set>
#include <list>

#include <gl/glew.h>

class RageDisplay_New final: public RageDisplay
{
public:
	RageDisplay_New();
	virtual ~RageDisplay_New();

	RString Init( const VideoModeParams &p, bool bAllowUnacceleratedRenderer ) override;

	RString GetApiDescription() const override { return "Gaz's amazing new renderer"; }
	void GetDisplaySpecs(DisplaySpecs &out) const override;
	const RagePixelFormatDesc *GetPixelFormatDesc(RagePixelFormat pf) const override;

	bool BeginFrame() override;
	void EndFrame() override;

	ActualVideoModeParams GetActualVideoModeParams() const override { return mWindow->GetActualVideoModeParams(); }
	void ResolutionChanged() override;

	bool SupportsPerVertexMatrixScale() override {
		// TODO: I honestly don't understand what this is for, but mimic RageDisplay_Legacy
		// Yes this renderer most definitely supports VBOs
		return true;
	}
	bool SupportsTextureFormat(RagePixelFormat fmt, bool realtime = false) override;
	bool SupportsSurfaceFormat(RagePixelFormat fmt);
	bool SupportsThreadedRendering() override {
		// TODO: Not yet
		return false;
	}
	int GetNumTextureUnits() override {
	  // Far more than stepmania should ever need
		return mNumTextureUnits;
	}
	bool SupportsRenderToTexture() const override {
		// TODO: Not yet
		return false;
	}
	bool SupportsFullscreenBorderlessWindow() const override {
	  // TODO: Not yet
	  return false;
	}

	uintptr_t CreateTexture(
		RagePixelFormat fmt,
		RageSurface* img,
		bool generateMipMaps
	) override;
	void UpdateTexture(
		uintptr_t texHandle,
		RageSurface* img,
		int xoffset, int yoffset, int width, int height
	) override;
	void DeleteTexture( uintptr_t texHandle ) override;	
	void ClearAllTextures() override;
	void SetTexture( TextureUnit unit, uintptr_t texture ) override;
	void SetTextureMode( TextureUnit unit, TextureMode mode ) override;
	void SetTextureWrapping( TextureUnit unit, bool wrap ) override;
	int GetMaxTextureSize() const override;
	void SetTextureFiltering( TextureUnit unit, bool filter ) override;
	void SetSphereEnvironmentMapping(TextureUnit tu, bool enabled) override;

	RageTextureLock* CreateTextureLock() {
		// TODO: Seems to be used for pixel buffers
		//       or other glMapBuffer uploads.
		// Probably for updating textures in a semi-fast
		// way, i.e. video?
		return nullptr;
	}

	bool IsZTestEnabled() const override;
	bool IsZWriteEnabled() const override;
	void SetZWrite( bool enabled ) override;
	void SetZTestMode( ZTestMode mode ) override;
	void SetZBias( float bias ) override;
	void ClearZBuffer() override;

	void SetBlendMode( BlendMode mode ) override;
	void SetCullMode( CullMode mode ) override;
	void SetAlphaTest( bool enable ) override;

	void SetMaterial(
		const RageColor& emissive,
		const RageColor& ambient,
		const RageColor& diffuse,
		const RageColor& specular,
		float shininess
	) override;

	void SetLighting( bool enable ) override;
	void SetLightOff( int index ) override;
	void SetLightDirectional( 
		int index, 
		const RageColor& ambient, 
		const RageColor& diffuse, 
		const RageColor& specular, 
		const RageVector3& dir ) override;

	void SetEffectMode(EffectMode effect) override;
	bool IsEffectModeSupported(EffectMode effect) override;
	void SetCelShaded(int stage) override;

	RageCompiledGeometry* CreateCompiledGeometry() override;
	void DeleteCompiledGeometry( RageCompiledGeometry* geom ) override;

protected:
	class FrameBuffer
	{
		public:
			FrameBuffer(GLuint width, GLuint height, GLint tempTexUnit);
			~FrameBuffer();
			void bindRenderTarget();
			void unbindRenderTarget();
			void bindTexture(GLint texUnit);

			GLuint width() const { return mWidth; }
			GLuint height() const { return mHeight; }

		private:
			GLuint mFBO = 0;
			GLuint mTex = 0;
			GLuint mDepthRBO = 0;
			GLuint mWidth = 0;
			GLuint mHeight = 0;
  };

	enum class ShaderName
	{
		RenderPlaceholder,
		RenderPlaceholderCompiledGeometry,
		FlipFlopFinal,
		// TODO: All of these
		TextureMatrixScaling,
		Shell,
		Cel,
		DistanceField,
		Unpremultiply,
		ColourBurn,
		ColourDodge,
		VividLight,
		HardMix,
		Overlay,
		Screen,
		YUYV422,
	};

	void DrawQuadsInternal( const RageSpriteVertex v[], int numVerts ) override;
	void DrawQuadStripInternal( const RageSpriteVertex v[], int numVerts) override;
	void DrawFanInternal( const RageSpriteVertex v[], int numVerts) override;
	void DrawStripInternal( const RageSpriteVertex v[], int numVerts) override;
	void DrawTrianglesInternal( const RageSpriteVertex v[], int numVerts) override;
	void DrawCompiledGeometryInternal( const RageCompiledGeometry *p, int meshIndex) override;
	void DrawLineStripInternal( const RageSpriteVertex v[], int numVerts, float lineWidth ) override;
	void DrawSymmetricQuadStripInternal( const RageSpriteVertex v[], int numVerts) override;

	RString TryVideoMode( const VideoModeParams& p, bool& newDeviceCreated ) override final;

	RageSurface* CreateScreenshot();
	// RageMatrix GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf );

	void LoadShaderPrograms(bool failOnError = true);
	void LoadShaderProgram(ShaderName name, std::string vert, std::string frag, bool failOnError);
  GLuint LoadShader(GLenum type, std::string source, bool failOnError);
  bool UseProgram(ShaderName name);
  void InitVertexAttribsSpriteVertex();
  void SetShaderUniforms(bool enableVertexColour = true, bool enableVertexTextureMatrixScale = false);
  ShaderName effectModeToShaderName(EffectMode effect);

  void flipflopFBOs();
  void flipflopRender();
  void flipflopRenderInit();
  void flipflopRenderDeInit();

  LowLevelWindow* mWindow = nullptr;

  // TODO: Invalid enum errors on nviia
  const bool frameSyncUsingFences = false;

  // GL Fences to allow a desired frames-in-flight, but
  // without a large stall from glFinish()
  // TODO: See comments in EndFrame - There's valid arguments for and against this
  const uint32_t frameSyncDesiredFramesInFlight = 2;
  std::list<GLsync> frameSyncFences;

  ZTestMode mZTestMode = ZTestMode::ZTEST_OFF; // GL_DEPTH_TEST
  bool mZWriteEnabled = true; // glDepthMask
  float mFNear = 0.0;
  float mFFar = 0.0;

  std::set<GLuint> mTextures;
  GLint mNumTextureUnits = 0;
  GLint mNumTextureUnitsCombined = 0;
  GLint mTextureUnitForTexUploads = 0;
  GLint mTextureUnitForFBOUploads = 0;
  GLint mTextureUnitForFlipFlopRender = 0;

  // Texturing / blending settings on a per-texture-unit basis
  struct TextureUnitSettings
  {
    bool enabled = false;
	  TextureMode textureMode = TextureMode::TextureMode_Modulate;
	  // TODO: Combine / glow support
  };
  std::map<TextureUnit, TextureUnitSettings> mTextureSettings;

  std::map<ShaderName, GLuint> mShaderPrograms;
  std::pair<ShaderName, GLuint> mActiveShaderProgram;

  RageColor mMaterialEmissive;
  RageColor mMaterialAmbient;
  RageColor mMaterialDiffuse;
  RageColor mMaterialSpecular;
  float mMaterialShininess;

  bool mAlphaTestEnabled = false;
  float mAlphaTestThreshold = 0.0f;

  // TODO: The 2nd buffer here may not be needed it seems,
  //       though render to texture and preprocessing support
  //       would be handy to have, since it's essentially
  //       free.
  // Internal buffers, allowing read from previous frame
  // Flip: Render target, output of current frame
  // Flop: Previous frame, input to frag shader
  // std::unique_ptr<FrameBuffer> mFlip;
  // std::unique_ptr<FrameBuffer> mFlop;
  // GLuint mFlipflopRenderVAO = 0;
  // GLuint mFlipflopRenderVBO = 0;

  // TODO: Pending rewrite of DrawLineStrip
  float mLineWidthRange[2] = {0.0f, 50.0f};
  float mPointSizeRange[2] = {0.0f, 50.0f};

  std::set<RageCompiledGeometryNew*> mCompiledGeometry;
};

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
