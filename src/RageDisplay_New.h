/* RageDisplay_New

This is a likely misguided effort to rewrite the renderer with _some_ modern graphics tech.

Initially this is based on RageDisplay_Legacy (OpenGL), but isn't guaranteed to use GL going forward

I will probably fail - Gaz
*/

#pragma once

#include <arch/LowLevelWindow/LowLevelWindow.h>

#include <memory>
#include <set>

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

	bool BeginFrame() override final;
	void EndFrame() override final;

	ActualVideoModeParams GetActualVideoModeParams() const override { return mVideoParams; }

	bool SupportsPerVertexMatrixScale() override { return false; }
	bool SupportsTextureFormat(RagePixelFormat, bool /* realtime */ = false) override { return true; }
	bool SupportsThreadedRendering() override { return false; }
	int GetNumTextureUnits() override { return mNumTextureUnits; }
	bool SupportsRenderToTexture() const override { return false; }
	bool SupportsFullscreenBorderlessWindow() const override { return false; }

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

	RageTextureLock* CreateTextureLock() { return nullptr; }

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

	void SetSphereEnvironmentMapping( TextureUnit /* tu */, bool /* b */ ) { }
	void SetCelShaded( int /* stage */ ) { }

	RageCompiledGeometry* CreateCompiledGeometry();
	void DeleteCompiledGeometry( RageCompiledGeometry* );

protected:
	enum class ShaderName
	{
		RenderPlaceholder,
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

	void DrawQuadsInternal( const RageSpriteVertex v[], int numVerts );
	void DrawQuadStripInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) { }
	void DrawFanInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) { }
	void DrawStripInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) { }
	void DrawTrianglesInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) { }
	void DrawCompiledGeometryInternal( const RageCompiledGeometry *p, int /* iMeshIndex */ ) { }
	void DrawLineStripInternal( const RageSpriteVertex v[], int /* iNumVerts */, float /* LineWidth */ ) { }
	void DrawSymmetricQuadStripInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) { }

	RString TryVideoMode( const VideoModeParams& p, bool& newDeviceCreated ) override final;

	RageSurface* CreateScreenshot();
	// RageMatrix GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf );

  // TODO: Investigate all this stuff...
	bool SupportsSurfaceFormat( RagePixelFormat ) { return true; }

	void LoadShaderPrograms();
	void LoadShaderProgram(ShaderName name, std::string vert, std::string frag);
  GLuint LoadShader(GLenum type, std::string source);
  void UseProgram(ShaderName name);
  void InitVertexAttribsSpriteVertex();
  void SetShaderUniforms();

  LowLevelWindow* mWindow = nullptr;
  VideoModeParams mVideoParams;

  ZTestMode mZTestMode = ZTestMode::ZTEST_OFF; // GL_DEPTH_TEST
  bool mZWriteEnabled = true; // glDepthMask
  float mFNear = 0.0;
  float mFFar = 0.0;

  std::set<GLuint> mTextures;
  GLint mNumTextureUnits = 0;
  GLint mNumTextureUnitsCombined = 0;

  std::map<ShaderName, GLuint> mShaderPrograms;
  GLuint mActiveShaderProgram = 0;

  RageColor mMaterialEmissive;
  RageColor mMaterialAmbient;
  RageColor mMaterialDiffuse;
  RageColor mMaterialSpecular;
  float mMaterialShininess;
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
