#version 400 core

in vec4 vC;
in vec2 vT;

uniform sampler2D texture0;
uniform bool texture0Enabled;
uniform sampler2D texture1;
uniform bool texture1Enabled;

out vec4 fragColour;

vec4 ApplyScreen( vec4 over, vec4 under, float fill )
{
	vec3 NeutralColor = vec3(0.5,0.5,0.5);
	over.rgb = mix( NeutralColor, over.rgb, fill );

	vec4 ret;
	ret.rgb = (1.0 - ((1.0 - under.rgb) * (1.0 - over.rgb)));

	ret.rgb = min(ret.rgb, 1.0);
	ret.rgb *= over.a * under.a;

	ret.a = over.a * under.a;
	return ret;
}

void main(void)
{
	if( !texture0Enabled || !texture1Enabled ) {
		// Shouldn't be possible
		discard;
	}

	// creates two vec4s that represent the two textures.
	vec4 under = texture( texture0, vT );
	vec4 over = texture( texture1, vT );

	// the return value is also a vec4.
	vec4 ret = ApplyScreen( over, under, vC.a );

	// glenn does some math here that I haven't put the time in to understand yet.
	ret.rgb += (1.0 - over.a) * under.rgb * under.a;
	ret.a += (1.0 - over.a) * under.a;

	over.a *= vC.a;
	ret.rgb += (1.0 - under.a) * over.rgb * over.a;
	ret.a += (1.0 - under.a) * over.a;

	// this is unpremultiply though:
	ret.rgb /= ret.a;

	fragColour = ret;
}

/*
 * Copyright (c) 2009 AJ Kelly
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
