
const float distanceThreshold = 0.5;

float aastep(float threshold, float dist) {
	float afwidth = 0.75 * length(vec2(dFdx(dist), dFdy(dist)));
	return smoothstep(threshold - afwidth, threshold + afwidth, dist);
}

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

float sampleTexture(vec2 tc) {
	vec3 texel = texture(texture0, tc).rgb;
	float dist = median(texel.r, texel.g, texel.b);
	return aastep(distanceThreshold, dist);
}

vec4 effectMode(vec4 c, vec2 uv) { 
	if( !texture0Enabled ) {
		// Shouldn't be possible
		discard;
	}

	float dfValue = sampleTexture(uv.st);
	return vec4(c.rgb, c.a * dfValue);
}
