#include "global.h"
#include "RageDisplay.h"
#include "RageTimer.h"
#include "RageLog.h"
#include "RageMath.h"
#include "RageUtil.h"
#include "RageFile.h"
#include "RageSurface_Save_BMP.h"
#include "RageSurface_Save_JPEG.h"
#include "RageSurface_Save_PNG.h"
#include "RageSurfaceUtils_Zoom.h"
#include "RageSurface.h"
#include "Preference.h"
#include "LocalizedString.h"
#include "DisplaySpec.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "RageMatrices.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>


// Statistics stuff
RageTimer	g_LastCheckTimer;
int		g_iNumVerts;
int		g_iFPS, g_iVPF, g_iCFPS;

int RageDisplay::GetFPS() const { return g_iFPS; }
int RageDisplay::GetVPF() const { return g_iVPF; }
int RageDisplay::GetCumFPS() const { return g_iCFPS; }

static int g_iFramesRenderedSinceLastCheck,
	   g_iFramesRenderedSinceLastReset,
	   g_iVertsRenderedSinceLastCheck,
	   g_iNumChecksSinceLastReset;
static RageTimer g_LastFrameEndedAt( RageZeroTimer );

RageDisplay*		DISPLAY	= nullptr; // global and accessible from anywhere in our program

Preference<bool>  LOG_FPS( "LogFPS", true );
Preference<float> g_fFrameLimitPercent( "FrameLimitPercent", 0.0f );

static const char *RagePixelFormatNames[] = {
	"RGBA8",
	"BGRA8",
	"RGBA4",
	"RGB5A1",
	"RGB5",
	"RGB8",
	"PAL",
	"BGR8",
	"A1BGR5",
	"X1RGB5",
};
XToString( RagePixelFormat );

/* bNeedReloadTextures is set to true if the device was re-created and we need
 * to reload textures.  On failure, an error message is returned.
 * XXX: the renderer itself should probably be the one to try fallback modes */
static LocalizedString SETVIDEOMODE_FAILED ( "RageDisplay", "SetVideoMode failed:" );
RString RageDisplay::SetVideoMode( VideoModeParams p, bool &bNeedReloadTextures )
{
	RString err;
	std::vector<RString> vs;

	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	LOG->Trace( "TryVideoMode failed: %s", err.c_str() );
	vs.push_back( err );

	// fall back to settings that will most likely work
	p.bpp = 16;
	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	vs.push_back( err );

	// "Intel(R) 82810E Graphics Controller" won't accept a 16 bpp surface if
	// the desktop is 32 bpp, so try 32 bpp as well.
	p.bpp = 32;
	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	vs.push_back( err );

	// Fall back on a known resolution good rather than 640 x 480.
	DisplaySpecs dr;
	this->GetDisplaySpecs(dr);
	if( dr.empty() )
	{
		vs.push_back( "No display resolutions" );
		return SETVIDEOMODE_FAILED.GetValue() + " " + join(";",vs);
	}

	DisplaySpec d =  *dr.begin();
	// Try to find DisplaySpec corresponding to requested display
	for (const auto &candidate: dr)
	{
		if (candidate.currentMode() != nullptr)
		{
			d = candidate;
			if (candidate.id() == p.sDisplayId)
			{
				break;
			}
		}
	}

	p.sDisplayId = d.id();
	const DisplayMode supported = d.currentMode() != nullptr ? *d.currentMode() : *d.supportedModes().begin();
	p.width = supported.width;
	p.height = supported.height;
	p.rate = std::round(supported.refreshRate);
	if( (err = this->TryVideoMode(p,bNeedReloadTextures)) == "" )
		return RString();
	vs.push_back( err );

	return SETVIDEOMODE_FAILED.GetValue() + " " + join(";",vs);
}

void RageDisplay::ProcessStatsOnFlip()
{
	g_iFramesRenderedSinceLastCheck++;
	g_iFramesRenderedSinceLastReset++;

	if( g_LastCheckTimer.PeekDeltaTime() >= 1.0f )	// update stats every 1 sec.
	{
		float fActualTime = g_LastCheckTimer.GetDeltaTime();
		g_iNumChecksSinceLastReset++;
		g_iFPS = std::lrint( g_iFramesRenderedSinceLastCheck / fActualTime );
		g_iCFPS = g_iFramesRenderedSinceLastReset / g_iNumChecksSinceLastReset;
		g_iCFPS = std::lrint( g_iCFPS / fActualTime );
		g_iVPF = g_iVertsRenderedSinceLastCheck / g_iFramesRenderedSinceLastCheck;
		g_iFramesRenderedSinceLastCheck = g_iVertsRenderedSinceLastCheck = 0;
		if( LOG_FPS )
		{
			RString sStats = GetStats();
			sStats.Replace( "\n", ", " );
			LOG->Trace( "%s", sStats.c_str() );
		}
	}
}

void RageDisplay::ResetStats()
{
	g_iFPS = g_iVPF = 0;
	g_iFramesRenderedSinceLastCheck = g_iFramesRenderedSinceLastReset = 0;
	g_iNumChecksSinceLastReset = 0;
	g_iVertsRenderedSinceLastCheck = 0;
	g_LastCheckTimer.GetDeltaTime();
}

RString RageDisplay::GetStats() const
{
	RString s;
	// If FPS == 0, we don't have stats yet.
	if( !GetFPS() )
		s = "-- FPS\n-- av FPS\n-- VPF";

	s = ssprintf( "%i FPS\n%i av FPS\n%i VPF", GetFPS(), GetCumFPS(), GetVPF() );

//	#if defined(_WINDOWS)
	s += "\n"+this->GetApiDescription();
//	#endif

	return s;
}

bool RageDisplay::BeginFrame()
{
	this->SetDefaultRenderStates();

	return true;
}

void RageDisplay::EndFrame()
{
	ProcessStatsOnFlip();
}

void RageDisplay::BeginConcurrentRendering()
{
	this->SetDefaultRenderStates();
}

void RageDisplay::StatsAddVerts( int iNumVertsRendered ) { g_iVertsRenderedSinceLastCheck += iNumVertsRendered; }

/* Draw a line as a quad.  GL_LINES with SmoothLines off can draw line
 * ends at odd angles--they're forced to axis-alignment regardless of the
 * angle of the line. */
void RageDisplay::DrawPolyLine(const RageSpriteVertex &p1, const RageSpriteVertex &p2, float LineWidth )
{
	// soh cah toa strikes strikes again!
	float opp = p2.p.x - p1.p.x;
	float adj = p2.p.y - p1.p.y;
	float hyp = std::pow(opp*opp + adj*adj, 0.5f);

	float lsin = opp/hyp;
	float lcos = adj/hyp;

	RageSpriteVertex v[4];

	v[0] = v[1] = p1;
	v[2] = v[3] = p2;

	float ydist = lsin * LineWidth/2;
	float xdist = lcos * LineWidth/2;

	v[0].p.x += xdist;
	v[0].p.y -= ydist;
	v[1].p.x -= xdist;
	v[1].p.y += ydist;
	v[2].p.x -= xdist;
	v[2].p.y += ydist;
	v[3].p.x += xdist;
	v[3].p.y -= ydist;

	this->DrawQuad(v);
}


void RageDisplay::DrawLineStripInternal( const RageSpriteVertex v[], int iNumVerts, float LineWidth )
{
	ASSERT( iNumVerts >= 2 );

	/* Draw a line strip with rounded corners using polys. This is used on
	 * cards that have strange allergic reactions to antialiased points and
	 * lines. */
	for( int i = 0; i < iNumVerts-1; ++i )
		DrawPolyLine(v[i], v[i+1], LineWidth);

	// Join the lines with circles so we get rounded corners.
	for( int i = 0; i < iNumVerts; ++i )
		DrawCircle( v[i], LineWidth/2 );
}

void RageDisplay::DrawCircleInternal( const RageSpriteVertex &p, float radius )
{
	const int subdivisions = 32;
	RageSpriteVertex v[subdivisions+2];
	v[0] = p;

	for(int i = 0; i < subdivisions+1; ++i)
	{
		const float fRotation = float(i) / subdivisions * 2*PI;
		const float fX = RageFastCos(fRotation) * radius;
		const float fY = -RageFastSin(fRotation) * radius;
		v[1+i] = v[0];
		v[1+i].p.x += fX;
		v[1+i].p.y += fY;
	}

	this->DrawFan( v, subdivisions+2 );
}


void RageDisplay::SetDefaultRenderStates()
{
	SetLighting( false );
	SetCullMode( CULL_NONE );
	SetZWrite( false );
	SetZTestMode( ZTEST_OFF );
	SetAlphaTest( true );
	SetBlendMode( BLEND_NORMAL );
	SetTextureFiltering( TextureUnit_1, true );
	SetZBias( 0 );
	LoadMenuPerspective( 0, 640, 480, 320, 240 ); // 0 FOV = ortho
}

RageDisplay::RageDisplay()
{
	RageMatrices::UpdateCentering();

	// Register with Lua.
	{
		Lua *L = LUA->Get();
		lua_pushstring( L, "DISPLAY" );
		this->PushSelf( L );
		lua_settable( L, LUA_GLOBALSINDEX );
		LUA->Release( L );
	}
}

RageDisplay::~RageDisplay()
{
	// Unregister with Lua.
	LUA->UnsetGlobal( "DISPLAY" );
}

RageSurface *RageDisplay::CreateSurfaceFromPixfmt( RagePixelFormat pixfmt,
						void *pixels, int width, int height, int pitch )
{
	const RagePixelFormatDesc *tpf = GetPixelFormatDesc(pixfmt);

	RageSurface *surf = CreateSurfaceFrom(
		width, height, tpf->bpp,
		tpf->masks[0], tpf->masks[1], tpf->masks[2], tpf->masks[3],
		(std::uint8_t *) pixels, pitch );

	return surf;
}

RagePixelFormat RageDisplay::FindPixelFormat( int iBPP, unsigned iRmask, unsigned iGmask, unsigned iBmask, unsigned iAmask, bool bRealtime )
{
	RagePixelFormatDesc tmp = { iBPP, { iRmask, iGmask, iBmask, iAmask } };

	FOREACH_ENUM( RagePixelFormat, iPixFmt )
	{
		const RagePixelFormatDesc *pf = GetPixelFormatDesc( RagePixelFormat(iPixFmt) );
		if( !SupportsTextureFormat(RagePixelFormat(iPixFmt), bRealtime) )
			continue;

		if( memcmp(pf, &tmp, sizeof(tmp)) )
			continue;
		return iPixFmt;
	}

	return RagePixelFormat_Invalid;
}

RageMatrix RageDisplay::GetOrthoMatrix( float l, float r, float b, float t, float zn, float zf )
{
	return RageMatrices::GetOrthoMatrixGL(l, r, b, t, zn, zf);
}

void RageDisplay::LoadMenuPerspective( float fovDegrees, float fWidth, float fHeight, float fVanishPointX, float fVanishPointY )
{
	RageMatrices::LoadMenuPerspective( RageMatrices::GraphicsProjectionMode::OpenGL,
		fovDegrees, fWidth, fHeight, fVanishPointX, fVanishPointY);
}

void RageDisplay::ResolutionChanged()
{
	// The centering matrix depends on the resolution.
	auto p = GetActualVideoModeParams();
	RageMatrices::UpdateDisplayParameters(p.width, p.height, p.fDisplayAspectRatio);
	RageMatrices::UpdateCentering();
}

bool RageDisplay::SaveScreenshot( RString sPath, GraphicsFileFormat format )
{
	RageTimer timer;
	RageSurface *surface = this->CreateScreenshot();
//	LOG->Trace( "CreateScreenshot took %f seconds", timer.GetDeltaTime() );

	if (nullptr == surface)
	{
		LOG->Trace("CreateScreenshot failed to return a surface");
		return false;
	}

	/* Unless we're in lossless, resize the image to 640x480.  If we're saving lossy,
	 * there's no sense in saving 1280x960 screenshots, and we don't want to output
	 * screenshots in a strange (non-1) sample aspect ratio. */
	if( format != SAVE_LOSSLESS && format != SAVE_LOSSLESS_SENSIBLE )
	{
		// Maintain the DAR.
		ASSERT( GetActualVideoModeParams().fDisplayAspectRatio > 0 );
		int iHeight = 480;
		// This used to be lrint. However, lrint causes odd resolutions like
		// 639x480 (4:3) and 853x480 (16:9). ceil gives correct values. -aj
		int iWidth = std::ceil( iHeight * GetActualVideoModeParams().fDisplayAspectRatio );
		timer.Touch();
		RageSurfaceUtils::Zoom( surface, iWidth, iHeight );
//		LOG->Trace( "%ix%i -> %ix%i (%.3f) in %f seconds", surface->w, surface->h, iWidth, iHeight, GetActualVideoModeParams().fDisplayAspectRatio, timer.GetDeltaTime() );
	}

	RageFile out;
	if( !out.Open( sPath, RageFile::WRITE ) )
	{
		LOG->Trace("Couldn't write %s: %s", sPath.c_str(), out.GetError().c_str() );
		SAFE_DELETE( surface );
		return false;
	}

	bool bSuccess = false;
	timer.Touch();
	RString strError = "";
	switch( format )
	{
	case SAVE_LOSSLESS:
		bSuccess = RageSurfaceUtils::SaveBMP( surface, out );
		break;
	case SAVE_LOSSLESS_SENSIBLE:
		bSuccess = RageSurfaceUtils::SavePNG( surface, out, strError );
		break;
	case SAVE_LOSSY_LOW_QUAL:
		bSuccess = RageSurfaceUtils::SaveJPEG( surface, out, false );
		break;
	case SAVE_LOSSY_HIGH_QUAL:
		bSuccess = RageSurfaceUtils::SaveJPEG( surface, out, true );
		break;
	DEFAULT_FAIL( format );
	}
//	LOG->Trace( "Saving Screenshot file took %f seconds.", timer.GetDeltaTime() );

	SAFE_DELETE( surface );

	if( !bSuccess )
	{
		LOG->Trace("Couldn't write %s: %s", sPath.c_str(), out.GetError().c_str() );
		return false;
	}

	return true;
}

void RageDisplay::DrawQuads( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( (iNumVerts%4) == 0 );

	if(!iNumVerts)
		return;

	this->DrawQuadsInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawQuadStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( (iNumVerts%2) == 0 );

	if(iNumVerts < 4)
		return;

	this->DrawQuadStripInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawFan( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );

	this->DrawFanInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );

	this->DrawStripInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawTriangles( const RageSpriteVertex v[], int iNumVerts )
{
	if( iNumVerts == 0 )
		return;

	ASSERT( iNumVerts >= 3 );

	this->DrawTrianglesInternal(v,iNumVerts);

	StatsAddVerts(iNumVerts);
}

void RageDisplay::DrawCompiledGeometry( const RageCompiledGeometry *p, int iMeshIndex, const std::vector<msMesh> &vMeshes )
{
	this->DrawCompiledGeometryInternal( p, iMeshIndex );

	StatsAddVerts( vMeshes[iMeshIndex].Triangles.size() );
}

void RageDisplay::DrawLineStrip( const RageSpriteVertex v[], int iNumVerts, float LineWidth )
{
	ASSERT( iNumVerts >= 2 );

	this->DrawLineStripInternal( v, iNumVerts, LineWidth );
}

/*
 * Draw a strip of:
 *
 * 0..1..2
 * . /.\ .
 * ./ . \.
 * 3..4..5
 * . /.\ .
 * ./ . \.
 * 6..7..8
 */

void RageDisplay::DrawSymmetricQuadStrip( const RageSpriteVertex v[], int iNumVerts )
{
	ASSERT( iNumVerts >= 3 );

	if( iNumVerts < 6 )
		return;

	this->DrawSymmetricQuadStripInternal( v, iNumVerts );

	StatsAddVerts( iNumVerts );
}

void RageDisplay::DrawCircle( const RageSpriteVertex &v, float radius )
{
	this->DrawCircleInternal( v, radius );
}

void RageDisplay::FrameLimitBeforeVsync( int iFPS )
{
	ASSERT( iFPS != 0 );

	int iDelayMicroseconds = 0;
	if( g_fFrameLimitPercent.Get() > 0.0f && !g_LastFrameEndedAt.IsZero() )
	{
		float fFrameTime = g_LastFrameEndedAt.GetDeltaTime();
		float fExpectedTime = 1.0f / iFPS;

		/* This is typically used to turn some of the delay that would normally
		 * be waiting for vsync and turn it into a usleep, to make sure we give
		 * up the CPU.  If we overshoot the sleep, then we'll miss the vsync,
		 * so allow tweaking the amount of time we expect a frame to take.
		 * Frame limiting is disabled by setting this to 0. */
		fExpectedTime *= g_fFrameLimitPercent.Get();
		float fExtraTime = fExpectedTime - fFrameTime;

		iDelayMicroseconds = int(fExtraTime * 1000000);
	}

	if( !HOOKS->AppHasFocus() )
		iDelayMicroseconds = std::max( iDelayMicroseconds, 10000 ); // give some time to other processes and threads

	if( iDelayMicroseconds > 0 )
		usleep( iDelayMicroseconds );
}

void RageDisplay::FrameLimitAfterVsync()
{
	if( g_fFrameLimitPercent.Get() == 0.0f )
		return;

	g_LastFrameEndedAt.Touch();
}


RageCompiledGeometry::~RageCompiledGeometry()
{
	m_bNeedsNormals = false;
}

void RageCompiledGeometry::Set( const std::vector<msMesh> &vMeshes, bool bNeedsNormals )
{
	m_bNeedsNormals = bNeedsNormals;

	std::size_t totalVerts = 0;
	std::size_t totalTriangles = 0;

	m_bAnyNeedsTextureMatrixScale = false;

	m_vMeshInfo.resize( vMeshes.size() );
	for( unsigned i=0; i<vMeshes.size(); i++ )
	{
		const msMesh& mesh = vMeshes[i];
		const std::vector<RageModelVertex> &Vertices = mesh.Vertices;
		const std::vector<msTriangle> &Triangles = mesh.Triangles;

		MeshInfo& meshInfo = m_vMeshInfo[i];
		meshInfo.m_bNeedsTextureMatrixScale = false;

		meshInfo.iVertexStart = totalVerts;
		meshInfo.iVertexCount = Vertices.size();
		meshInfo.iTriangleStart = totalTriangles;
		meshInfo.iTriangleCount = Triangles.size();

		totalVerts += Vertices.size();
		totalTriangles += Triangles.size();

		for( unsigned j = 0; j < Vertices.size(); ++j )
		{
			if( Vertices[j].TextureMatrixScale.x != 1.0f || Vertices[j].TextureMatrixScale.y != 1.0f )
			{
				meshInfo.m_bNeedsTextureMatrixScale = true;
				m_bAnyNeedsTextureMatrixScale = true;
			}
		}
	}

	this->Allocate( vMeshes );

	Change( vMeshes );
}

// lua start
#include "LuaBinding.h"

// Register with Lua.
static void register_REFRESH_DEFAULT(lua_State *L)
{
	lua_pushstring( L, "REFRESH_DEFAULT" );
	lua_pushinteger( L, REFRESH_DEFAULT );
	lua_settable( L, LUA_GLOBALSINDEX);
}
REGISTER_WITH_LUA_FUNCTION( register_REFRESH_DEFAULT );


/** @brief Allow Lua to have access to the RageDisplay. */
class LunaRageDisplay: public Luna<RageDisplay>
{
public:
	static int GetDisplayWidth( T* p, lua_State *L )
	{
		VideoModeParams params = p->GetActualVideoModeParams();
		LuaHelpers::Push( L, params.width );
		return 1;
	}

	static int GetDisplayHeight( T* p, lua_State *L )
	{
		VideoModeParams params = p->GetActualVideoModeParams();
		LuaHelpers::Push( L, params.height );
		return 1;
	}

	static int GetFPS( T* p, lua_State *L )
	{
		lua_pushnumber(L, p->GetFPS());
		return 1;
	}

	static int GetVPF( T* p, lua_State *L )
	{
		lua_pushnumber(L, p->GetVPF());
		return 1;
	}

	static int GetCumFPS( T* p, lua_State *L )
	{
		lua_pushnumber(L, p->GetCumFPS());
		return 1;
	}

	static int GetDisplaySpecs( T* p, lua_State *L )
	{
		DisplaySpecs s;
		p->GetDisplaySpecs(s);
		pushDisplaySpecs(L, s);
		return 1;
	}

	static int SupportsRenderToTexture( T* p, lua_State *L )
	{
		lua_pushboolean(L, p->SupportsRenderToTexture());
		return 1;
	}

	static int SupportsFullscreenBorderlessWindow( T* p, lua_State *L )
	{
		lua_pushboolean(L, p->SupportsFullscreenBorderlessWindow());
		return 1;
	}

	LunaRageDisplay()
	{
		ADD_METHOD( GetDisplayWidth );
		ADD_METHOD( GetDisplayHeight );
		ADD_METHOD( GetFPS );
		ADD_METHOD( GetVPF );
		ADD_METHOD( GetCumFPS );
		ADD_METHOD( GetDisplaySpecs );
		ADD_METHOD( SupportsRenderToTexture );
		ADD_METHOD( SupportsFullscreenBorderlessWindow );
	}
};

LUA_REGISTER_CLASS( RageDisplay )
// lua end

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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

