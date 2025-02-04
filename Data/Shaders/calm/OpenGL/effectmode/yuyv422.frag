
/*
 * Convert from YUYV422 to RGB.
 *
 * This is used by MovieTexture_Generic.  The input texture is TextureWidth
 * texels wide, the output texture is TextureWidth*2 texels wide, and texels
 * are aligned.  We can use this to determine if the fragment we're generating
 * is even or odd.
 */
vec4 effectMode(vec4 c, vec2 uv) { 
	if( !texture0Enabled ) {
		// Shouldn't be possible
		discard;
	}

	// CALM TODO - Test this - Best guess from the gl1 and ES2 variants
	vec4 tex = vec4(uv, 0, 0);

	float fRealWidth = float(textureWidth);
	float fU = tex.x;

	/* Scale U to [0.25,fRealWidth+0.25]. */
	fU *= fRealWidth;

	/* Scale the U texture coordinate to the size of the destination texture,
	 * [0.5,fDestWidth+0.5]. */
	fU *= 2.0;

	/* Texture coordinates are center-aligned; an exact sample lies at 0.5,
	 * not 0.  Shift U from [0.5,fDestWidth+0.5] to [0,fDestWidth]. */
	fU -= 0.5;

	/* If this is an odd fragment, fOdd is 1. */
	float fOdd = mod(fU+0.0001, 2.0);

	/* Align fU to an even coordinate. */
	fU -= fOdd;

	/* Scale U back.  Because fU is aligned to an even coordinate, this
	 * will always be an exact sample. */
	fU += 0.5;
	fU /= 2.0;
	fU /= fRealWidth;

	tex.x = fU;

	vec4 yuyv = texture( texture0, tex.xy );

	vec3 yuv;
	if( fOdd <= 0.5 )
		yuv = yuyv.rga;
	else
		yuv = yuyv.bga;
	yuv -= vec3(16.0/255.0, 128.0/255.0, 128.0/255.0);

	mat3 conv = mat3(
		// Y     U (Cb)    V (Cr)
		1.1643,  0.000,    1.5958,  // R
		1.1643, -0.39173, -0.81290, // G
		1.1643,  2.017,    0.000);  // B

	return vec4(
		dot(yuv,conv[0]),
		dot(yuv,conv[1]),
		dot(yuv,conv[2]),
		1.0
	);
}

/*
 * Copyright (c) 2007 Glenn Maynard
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

