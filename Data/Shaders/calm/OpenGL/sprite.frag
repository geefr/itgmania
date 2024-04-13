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

uniform sampler2D texture0;

out vec4 fragColour;

void main() {
	// vec3 viewDir = normalize(-vP.xyz);
	// vec3 n = normalize(vN);

	// TODO ;)
	// vec4 c = texture(texture0, vT);
	vec4 c = vec4(0.2, 0.8, 0.2, 1.0);
	fragColour = c;
}
