/* RageDisplay_New

This is a likely misguided effort to rewrite the renderer with _some_ modern graphics tech.

Initially this is based on RageDisplay_Legacy (OpenGL), but isn't guaranteed to use GL going forward

I will probably fail - Gaz
*/

#pragma once

#include <memory>

#include <arch/LowLevelWindow/LowLevelWindow.h>

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

	void SetBlendMode( BlendMode ) { }
	bool SupportsTextureFormat( RagePixelFormat, bool /* realtime */ =false ) { return true; }

	bool SupportsPerVertexMatrixScale() { return false; }
	uintptr_t CreateTexture(
		RagePixelFormat,
		RageSurface* /* img */,
		bool /* bGenerateMipMaps */ ) { return 1; }
	void UpdateTexture(
		uintptr_t /* iTexHandle */,
		RageSurface* /* img */,
		int /* xoffset */, int /* yoffset */, int /* width */, int /* height */
		) { }
	void DeleteTexture( uintptr_t /* iTexHandle */ ) { }
	void ClearAllTextures() { }
	int GetNumTextureUnits() { return 1; }
	void SetTexture( TextureUnit, uintptr_t /* iTexture */ ) { }
	void SetTextureMode( TextureUnit, TextureMode ) { }
	void SetTextureWrapping( TextureUnit, bool ) { }
	int GetMaxTextureSize() const { return 2048; }
	void SetTextureFiltering( TextureUnit, bool ) { }
	bool IsZWriteEnabled() const { return false; }
	bool IsZTestEnabled() const { return false; }
	void SetZWrite( bool ) { }
	void SetZBias( float ) { }
	void SetZTestMode( ZTestMode ) { }
	void ClearZBuffer() { }
	void SetCullMode( CullMode ) { }
	void SetAlphaTest( bool ) { }
	void SetMaterial(
		const RageColor & /* unreferenced: emissive */,
		const RageColor & /* unreferenced: ambient */,
		const RageColor & /* unreferenced: diffuse */,
		const RageColor & /* unreferenced: specular */,
		float /* unreferenced: shininess */
		) { }
	void SetLighting( bool ) { }
	void SetLightOff( int /* index */ ) { }
	void SetLightDirectional( 
		int /* index */, 
		const RageColor & /* unreferenced: ambient */, 
		const RageColor & /* unreferenced: diffuse */, 
		const RageColor & /* unreferenced: specular */, 
		const RageVector3 & /* unreferenced: dir */ ) { }

	void SetSphereEnvironmentMapping( TextureUnit /* tu */, bool /* b */ ) { }
	void SetCelShaded( int /* stage */ ) { }

	RageCompiledGeometry* CreateCompiledGeometry();
	void DeleteCompiledGeometry( RageCompiledGeometry* );

protected:
	void DrawQuadsInternal( const RageSpriteVertex v[], int /* iNumVerts */ ) { }
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

  LowLevelWindow* mWindow = nullptr;
  VideoModeParams mVideoParams;
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
