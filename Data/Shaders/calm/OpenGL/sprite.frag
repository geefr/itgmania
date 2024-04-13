#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;

const int MaxTextures = 4;

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 textureMat;
uniform bool enableTextureMatrixScale;
uniform vec2 textureMatrixScale;
uniform bool texture0Enabled;

uniform sampler2D texture0;


out vec4 fragColour;

void main() {
	// vec3 viewDir = normalize(-vP.xyz);
	// vec3 n = normalize(vN);

	// TODO: All the fancy texture blend modes and stuff - However that works
	// May be better as separate shaders, if such a thing is possible (perhaps to instance all the textured quads, then glow, etc)
	vec4 c = vec4(1.0, 1.0, 1.0, 1.0);

	if( texture0Enabled )
	{
		c = texture(texture0, vT);
		// c = vec4(1.0, 0.0, 1.0, 1.0);
	}
	else
	{
		c = vC;
	}

	// Modulate
	// c.rgb *= texture(texture0, vT).rgb;
	// c.a = 1.0;
	// c = texture(texture0, vT);

	fragColour = c;
}
