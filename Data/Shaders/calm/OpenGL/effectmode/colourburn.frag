
vec4 ApplyBurn( vec4 over, vec4 under, float fill )
{
	vec3 NeutralColor = vec3(1,1,1);
	over.rgb = mix( NeutralColor, over.rgb, fill );

	vec4 ret;
	ret.rgb = (over.rgb + under.rgb - 1.0);

	ret.rgb /= max(over.rgb, 0.000001);
	ret.rgb *= over.a*under.a;
	ret = clamp( ret, 0.0, 1.0 );

	ret.a = over.a * under.a;

	return ret;
}

vec4 effectMode(vec4 c, vec2 uv) { 
	if( !texture0Enabled || !texture1Enabled ) {
		// Shouldn't be possible
		discard;
	}

	vec4 under = texture( texture0, uv );
	vec4 over = texture( texture1, uv );

	vec4 ret = ApplyBurn( over, under, c.a );

	ret.rgb += (1.0 - over.a) * under.rgb * under.a;
	ret.a += (1.0 - over.a) * under.a;

	over.a *= c.a;
	ret.rgb += (1.0 - under.a) * over.rgb * over.a;
	ret.a += (1.0 - under.a) * over.a;

	ret.rgb /= ret.a;
	
	return ret;
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
