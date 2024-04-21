#include "global.h"

#include "RageMatrices.h"

RageMatrices::MatrixStack::MatrixStack()
{
    stack.resize(1);
    LoadIdentity();
}

void RageMatrices::MatrixStack::Pop()
{
    stack.pop_back();
    ASSERT( stack.size() > 0 ); // underflow
}

void RageMatrices::MatrixStack::Push()
{
    stack.push_back( stack.back() );
    ASSERT( stack.size() < 100 ); // overflow
}

void RageMatrices::MatrixStack::LoadIdentity()
{
    RageMatrixIdentity( &stack.back() );
}

void RageMatrices::MatrixStack::LoadMatrix( const RageMatrix& m )
{
    stack.back() = m;
}

void RageMatrices::MatrixStack::MultMatrix( const RageMatrix& m )
{
    RageMatrixMultiply( &stack.back(), &m, &stack.back() );
}

void RageMatrices::MatrixStack::MultMatrixLocal( const RageMatrix& m )
{
    RageMatrixMultiply( &stack.back(), &stack.back(), &m );
}

void RageMatrices::MatrixStack::RotateX( float degrees )
{
    RageMatrix m;
    RageMatrixRotationX( &m, degrees );
    MultMatrix( m );
}
void RageMatrices::MatrixStack::RotateY( float degrees )
{
    RageMatrix m;
    RageMatrixRotationY( &m, degrees );
    MultMatrix( m );
}
void RageMatrices::MatrixStack::RotateZ( float degrees )
{
    RageMatrix m;
    RageMatrixRotationZ( &m, degrees );
    MultMatrix( m );
}

void RageMatrices::MatrixStack::RotateXLocal( float degrees )
{
    RageMatrix m;
    RageMatrixRotationX( &m, degrees );
    MultMatrixLocal( m );
}
void RageMatrices::MatrixStack::RotateYLocal( float degrees )
{
    RageMatrix m;
    RageMatrixRotationY( &m, degrees );
    MultMatrixLocal( m );
}
void RageMatrices::MatrixStack::RotateZLocal( float degrees )
{
    RageMatrix m;
    RageMatrixRotationZ( &m, degrees );
    MultMatrixLocal( m );
}

void RageMatrices::MatrixStack::Scale( float x, float y, float z )
{
    RageMatrix m;
    RageMatrixScaling( &m, x, y, z );
    MultMatrix( m );
}

void RageMatrices::MatrixStack::ScaleLocal( float x, float y, float z )
{
    RageMatrix m;
    RageMatrixScaling( &m, x, y, z );
    MultMatrixLocal( m );
}

void RageMatrices::MatrixStack::Translate( float x, float y, float z )
{
    RageMatrix m;
    RageMatrixTranslation( &m, x, y, z );
    MultMatrix( m );
}

void RageMatrices::MatrixStack::TranslateLocal( float x, float y, float z )
{
    RageMatrix m;
    RageMatrixTranslation( &m, x, y, z );
    MultMatrixLocal( m );
}

void RageMatrices::MatrixStack::SkewX( float fAmount )
{
    RageMatrix m;
    RageMatrixSkewX( &m, fAmount );
    MultMatrixLocal( m );
}

void RageMatrices::MatrixStack::SkewY( float fAmount )
{
    RageMatrix m;
    RageMatrixSkewY( &m, fAmount );
    MultMatrixLocal( m );
}

const RageMatrix* RageMatrices::MatrixStack::GetTop() const
{
    return &stack.back();
}
void RageMatrices::MatrixStack::SetTop( const RageMatrix &m )
{
    stack.back() = m;
}

RageMatrices::RageMatrices() {
    RageMatrixIdentity(&centeringMatrix);
}

RageMatrices::~RageMatrices() {}

RageMatrices& RageMatrices::instance()
{
    static RageMatrices i;
    return i;
}

void RageMatrices::PushMatrix()
{
    instance().worldStack.Push();
}

void RageMatrices::PopMatrix()
{
	instance().worldStack.Pop();
}

void RageMatrices::Translate( float x, float y, float z )
{
	instance().worldStack.TranslateLocal(x, y, z);
}

void RageMatrices::TranslateWorld( float x, float y, float z )
{
	instance().worldStack.Translate(x, y, z);
}

void RageMatrices::Scale( float x, float y, float z )
{
	instance().worldStack.ScaleLocal(x, y, z);
}

void RageMatrices::RotateX( float deg )
{
	instance().worldStack.RotateXLocal( deg );
}

void RageMatrices::RotateY( float deg )
{
	instance().worldStack.RotateYLocal( deg );
}

void RageMatrices::RotateZ( float deg )
{
	instance().worldStack.RotateZLocal( deg );
}

void RageMatrices::SkewX( float fAmount )
{
	instance().worldStack.SkewX( fAmount );
}

void RageMatrices::SkewY( float fAmount )
{
	instance().worldStack.SkewY( fAmount );
}

void RageMatrices::MultMatrix( const RageMatrix &f )
{
    RageMatrices::PostMultMatrix(f);
}

void RageMatrices::PostMultMatrix( const RageMatrix &m )
{
	instance().worldStack.MultMatrix( m );
}

void RageMatrices::PreMultMatrix( const RageMatrix &m )
{
	instance().worldStack.MultMatrixLocal( m );
}

void RageMatrices::LoadIdentity()
{
	instance().worldStack.LoadIdentity();
}

void RageMatrices::TexturePushMatrix()
{
	instance().textureStack.Push();
}

void RageMatrices::TexturePopMatrix()
{
	instance().textureStack.Pop();
}

void RageMatrices::TextureTranslate( float x, float y )
{
	instance().textureStack.TranslateLocal(x, y, 0);
}

void RageMatrices::TextureTranslate( const RageVector2 &v )
{
    RageMatrices::TextureTranslate( v.x, v.y );
}

void RageMatrices::CameraPushMatrix()
{
	instance().projectionStack.Push();
	instance().viewStack.Push();
}

void RageMatrices::CameraPopMatrix()
{
	instance().projectionStack.Pop();
	instance().viewStack.Pop();
}

void RageMatrices::LoadMenuPerspective( GraphicsProjectionMode mode, float fovDegrees, float fWidth, float fHeight, float fVanishPointX, float fVanishPointY )
{
    auto& p = instance().projectionStack;
    auto& v = instance().viewStack;

	// fovDegrees == 0 gives ortho projection.
	if( fovDegrees == 0 )
	{
 		float left = 0, right = fWidth, bottom = fHeight, top = 0;
		switch(mode) {
			case GraphicsProjectionMode::OpenGL:
				p.LoadMatrix( GetOrthoMatrixGL(left, right, bottom, top, -1000, +1000) );
				break;
			case GraphicsProjectionMode::Direct3D:
				p.LoadMatrix( GetOrthoMatrixD3D(left, right, bottom, top, -1000, +1000) );
				break;
			default:
				ASSERT_M(false, "LoadMenuPerspective: Unknown graphics projection mode");
		}
 		v.LoadIdentity();
	}
	else
	{
		CLAMP( fovDegrees, 0.1f, 179.9f );
		float fovRadians = fovDegrees / 180.f * PI;
		float theta = fovRadians/2;
		float fDistCameraFromImage = fWidth/2 / std::tan( theta );

		fVanishPointX = SCALE( fVanishPointX, 0, fWidth, fWidth, 0 );
		fVanishPointY = SCALE( fVanishPointY, 0, fHeight, fHeight, 0 );

		fVanishPointX -= fWidth/2;
		fVanishPointY -= fHeight/2;

		// It's the caller's responsibility to push first.
        p.LoadMatrix(
            GetFrustumMatrix(
                (fVanishPointX-fWidth/2)/fDistCameraFromImage,
                (fVanishPointX+fWidth/2)/fDistCameraFromImage,
                (fVanishPointY+fHeight/2)/fDistCameraFromImage,
                (fVanishPointY-fHeight/2)/fDistCameraFromImage,
                1,
                fDistCameraFromImage+1000));

		v.LoadMatrix(
			RageLookAt(
				-fVanishPointX+fWidth/2, -fVanishPointY+fHeight/2, fDistCameraFromImage,
				-fVanishPointX+fWidth/2, -fVanishPointY+fHeight/2, 0,
				0.0f, 1.0f, 0.0f) );
	}
}

/* gluLookAt. The result is pre-multiplied to the matrix (M = L * M) instead of
 * post-multiplied. */
void RageMatrices::LoadLookAt( float fFOV, const RageVector3 &Eye, const RageVector3 &At, const RageVector3 &Up )
{
    auto& p = instance().projectionStack;
    auto& v = instance().viewStack;
	auto aspect = instance().aspect;
	p.LoadMatrix( GetPerspectiveMatrix(fFOV, aspect, 1, 1000) );

	// Flip the Y coordinate, so positive numbers go down.
	p.Scale( 1, -1, 1 );

	v.LoadMatrix( RageLookAt(Eye.x, Eye.y, Eye.z, At.x, At.y, At.z, Up.x, Up.y, Up.z) );
}

void RageMatrices::CenteringPushMatrix()
{
    auto& c = instance().centeringStack;
	c.push_back( c.back() );
	ASSERT( c.size() < 100 ); // overflow
}

void RageMatrices::CenteringPopMatrix()
{
    auto& c = instance().centeringStack;
	c.pop_back();
	ASSERT( c.size() > 0 ); // underflow
	UpdateCentering();
}

void RageMatrices::ChangeCentering(int iTranslateX, int iTranslateY, int iAddWidth, int iAddHeight )
{
	instance().centeringStack.back() = Centering( iTranslateX, iTranslateY, iAddWidth, iAddHeight );
	UpdateCentering();
}

RageMatrix RageMatrices::GetPerspectiveMatrix(float fovy, float aspect, float zNear, float zFar)
{
	float ymax = zNear * std::tan(fovy * PI / 360.0f);
	float ymin = -ymax;
	float xmin = ymin * aspect;
	float xmax = ymax * aspect;

	return GetFrustumMatrix(xmin, xmax, ymin, ymax, zNear, zFar);
}

/* These convert to OpenGL's coordinate system: -1,-1 is the bottom-left,
 * +1,+1 is the top-right, and Z goes from -1 (viewer) to +1 (distance).
 * It's a little odd, but very well-defined. */
RageMatrix RageMatrices::GetOrthoMatrixGL( float l, float r, float b, float t, float zn, float zf )
{
	RageMatrix m(
		2/(r-l),      0,            0,           0,
		0,            2/(t-b),      0,           0,
		0,            0,            -2/(zf-zn),   0,
		-(r+l)/(r-l), -(t+b)/(t-b), -(zf+zn)/(zf-zn),  1 );
	return m;
}

RageMatrix RageMatrices::GetFrustumMatrix( float l, float r, float b, float t, float zn, float zf )
{
	// glFrustum
	float A = (r+l) / (r-l);
	float B = (t+b) / (t-b);
	float C = -1 * (zf+zn) / (zf-zn);
	float D = -1 * (2*zf*zn) / (zf-zn);
	RageMatrix m(
		2*zn/(r-l), 0,          0,  0,
		0,          2*zn/(t-b), 0,  0,
		A,          B,          C,  -1,
		0,          0,          D,  0 );
	return m;
}

RageMatrix RageMatrices::GetOrthoMatrixD3D( float l, float r, float b, float t, float zn, float zf )
{
    auto m = GetOrthoMatrixGL(l, r, b, t, zn, zf);

	// Convert from OpenGL's [-1,+1] Z values to D3D's [0,+1].
	RageMatrix tmp;
	RageMatrixScaling( &tmp, 1, 1, 0.5f );
	RageMatrixMultiply( &m, &tmp, &m );

	RageMatrixTranslation( &tmp, 0, 0, 0.5f );
	RageMatrixMultiply( &m, &tmp, &m );

	return m;
}

RageMatrix RageMatrices::GetCenteringMatrix(float fTranslateX, float fTranslateY, float fAddWidth, float fAddHeight )
{
	// in screen space, left edge = -1, right edge = 1, bottom edge = -1. top edge = 1
	float fWidth = instance().width;
	float fHeight = instance().height;
	float fPercentShiftX = SCALE( fTranslateX, 0, fWidth, 0, +2.0f );
	float fPercentShiftY = SCALE( fTranslateY, 0, fHeight, 0, -2.0f );
	float fPercentScaleX = SCALE( fAddWidth, 0, fWidth, 1.0f, 2.0f );
	float fPercentScaleY = SCALE( fAddHeight, 0, fHeight, 1.0f, 2.0f );

	RageMatrix m1;
	RageMatrix m2;
	RageMatrixTranslation(
		&m1,
		fPercentShiftX,
		fPercentShiftY,
		0 );
	RageMatrixScaling(
		&m2,
		fPercentScaleX,
		fPercentScaleY,
		1 );
	RageMatrix mOut;
	RageMatrixMultiply( &mOut, &m1, &m2 );
	return mOut;
}

void RageMatrices::UpdateCentering()
{
	const Centering &p = instance().centeringStack.back();
	instance().centeringMatrix = GetCenteringMatrix(
		(float) p.m_iTranslateX, (float) p.m_iTranslateY, (float) p.m_iAddWidth, (float) p.m_iAddHeight );
}

void RageMatrices::UpdateDisplayParameters(int width, int height, float aspect) {
    instance().width = width;
    instance().height = height;
    instance().aspect = aspect;
}

const RageMatrix* RageMatrices::GetCentering()
{
	return &instance().centeringMatrix;
}

const RageMatrix* RageMatrices::GetProjectionTop()
{
	return instance().projectionStack.GetTop();
}

const RageMatrix* RageMatrices::GetViewTop()
{
	return instance().viewStack.GetTop();
}

const RageMatrix* RageMatrices::GetWorldTop()
{
	return instance().worldStack.GetTop();
}

const RageMatrix* RageMatrices::GetTextureTop()
{
	return instance().textureStack.GetTop();
}

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
