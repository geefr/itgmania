#version 400 core

in vec4 vC;
in vec2 vT;

uniform sampler2D texture0;
uniform bool texture0Enabled;

out vec4 fragColour;

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

void main(void)
{
	if( !texture0Enabled ) {
		// Shouldn't be possible
		discard;
	}

	float dfValue = sampleTexture(vT.st);
	fragColour = vec4(vC.rgb, vC.a * dfValue);
}
