#pragma once

#include "global.h"
#include "RageTypes.h"
#include "RageMath.h"

#include <vector>

/**
 * Matrix stacks (previously in RageDisplay)
 * 
 * Use these as static functions e.g. RageMatrixStack::PushMatrix();
 * Internally this class is a singleton, call RageMatrixStack::instance();
 */

class RageMatrices
{
    public:
        class MatrixStack
        {
            public:
                MatrixStack();
                // Pops the top of the stack.
                void Pop();
                // Pushes the stack by one, duplicating the current matrix.
                void Push();
                // Loads identity in the current matrix.
                void LoadIdentity();
                // Loads the given matrix into the current matrix
                void LoadMatrix( const RageMatrix& m );
                // Right-Multiplies the given matrix to the current matrix.
                // (transformation is about the current world origin)
                void MultMatrix( const RageMatrix& m );
                // Left-Multiplies the given matrix to the current matrix
                // (transformation is about the local origin of the object)
                void MultMatrixLocal( const RageMatrix& m );
                // Right multiply the current matrix with the computed rotation
                // matrix, counterclockwise about the given axis with the given angle.
                // (rotation is about the current world origin)
                void RotateX( float degrees );
                void RotateY( float degrees );
                void RotateZ( float degrees );
                // Left multiply the current matrix with the computed rotation
                // matrix. All angles are counterclockwise. (rotation is about the
                // local origin of the object)
                void RotateXLocal( float degrees );
                void RotateYLocal( float degrees );
                void RotateZLocal( float degrees );
                // Right multiply the current matrix with the computed scale
                // matrix. (transformation is about the current world origin)
                void Scale( float x, float y, float z );
                // Left multiply the current matrix with the computed scale
                // matrix. (transformation is about the local origin of the object)
                void ScaleLocal( float x, float y, float z );
                // Right multiply the current matrix with the computed translation
                // matrix. (transformation is about the current world origin)
                void Translate( float x, float y, float z );
                // Left multiply the current matrix with the computed translation
                // matrix. (transformation is about the local origin of the object)
                void TranslateLocal( float x, float y, float z );
                void SkewX( float fAmount );
                void SkewY( float fAmount );
                // Obtain the current matrix at the top of the stack
                const RageMatrix* GetTop() const;
                void SetTop( const RageMatrix &m );
            private:
                std::vector<RageMatrix> stack;
        };

        struct Centering
        {
            Centering( int iTranslateX = 0, int iTranslateY = 0, int iAddWidth = 0, int iAddHeight = 0 ):
                m_iTranslateX( iTranslateX ),
                m_iTranslateY( iTranslateY ),
                m_iAddWidth( iAddWidth ),
                m_iAddHeight( iAddHeight ) { }

            int m_iTranslateX, m_iTranslateY, m_iAddWidth, m_iAddHeight;
        };

        // Operating mode for a few matrix functions,
        // since RageDisplay_D3D requires alternate
        // projection matrices.
        enum class GraphicsProjectionMode {
            OpenGL,
            Direct3D
        };

        // World matrix stack functions.
        static void PushMatrix();
        static void PopMatrix();
        static void Translate( float x, float y, float z );
        static void TranslateWorld( float x, float y, float z );
        static void Scale( float x, float y, float z );
        static void RotateX( float deg );
        static void RotateY( float deg );
        static void RotateZ( float deg );
        static void SkewX( float fAmount );
        static void SkewY( float fAmount );
        static void MultMatrix( const RageMatrix &f );
        static void PostMultMatrix( const RageMatrix &f );
        static void PreMultMatrix( const RageMatrix &f );
        static void LoadIdentity();

        // Texture matrix functions
        static void TexturePushMatrix();
        static void TexturePopMatrix();
        static void TextureTranslate( float x, float y );
        static void TextureTranslate( const RageVector2 &v );

        // Projection and View matrix stack functions.
        static void CameraPushMatrix();
        static void CameraPopMatrix();

        static void LoadMenuPerspective( GraphicsProjectionMode mode, float fovDegrees, float fWidth, float fHeight, float fVanishPointX, float fVanishPointY );
        static void LoadLookAt(float fov, const RageVector3 &Eye, const RageVector3 &At, const RageVector3 &Up );

        // Centering matrix
        static void CenteringPushMatrix();
        static void CenteringPopMatrix();
        static void ChangeCentering(int trans_x, int trans_y, int add_width, int add_height );

        static RageMatrix GetPerspectiveMatrix( float fovy, float aspect, float zNear, float zFar );

        // Note: Ortho and perspective matrices are different between RageDisplay_OGL and RageDisplay_D3D
        // - These functions provide multiple conventions (and should be extended if further APIs are added)
        // - These should not be called directly, except from RageDisplay
        // - Elsewhere in the code, RageDisplay::GetOrthoMatrix should be used
        static RageMatrix GetOrthoMatrixGL( float l, float r, float b, float t, float zn, float zf );
        static RageMatrix GetOrthoMatrixD3D( float l, float r, float b, float t, float zn, float zf );

        static RageMatrix GetFrustumMatrix( float l, float r, float b, float t, float zn, float zf );

        // Matrix that adjusts position and scale of image on the screen
        static RageMatrix GetCenteringMatrix(float fTranslateX, float fTranslateY, float fAddWidth, float fAddHeight );
        static void UpdateCentering();

        // Required for centering matrix, updated via RageDisplay::ResolutionChanged
        static void UpdateDisplayParameters(int width, int height, float aspect);

        // Called by the RageDisplay derivitives
        static const RageMatrix* GetCentering();
        static const RageMatrix* GetProjectionTop();
        static const RageMatrix* GetViewTop();
        static const RageMatrix* GetWorldTop();
        static const RageMatrix* GetTextureTop();

    private:
        static RageMatrices& instance();
        RageMatrices();
        ~RageMatrices();

        std::vector<Centering> centeringStack = {{0, 0, 0, 0}};
        RageMatrix centeringMatrix;
        MatrixStack projectionStack;
        MatrixStack viewStack;
        MatrixStack worldStack;
        MatrixStack textureStack;

        int width = 1;
        int height = 1;
        float aspect = 1.0;
};

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