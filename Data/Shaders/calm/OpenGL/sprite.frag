#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;
in vec2 vB;

const int MaxTextures = 4;

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 textureMat;
uniform bool enableTextureMatrixScale;
uniform vec2 textureMatrixScale;
uniform bool texture0Enabled;
uniform vec4 fadeCoords; // left, bottom, top, right

uniform sampler2D texture0;

out vec4 fragColour;

vec4 textureMode_texture0(vec4 c, vec2 uv);
// vec4 textureMode_texture1(vec4 c, vec2 uv);
// vec4 textureMode_texture2(vec4 c, vec2 uv);
// vec4 textureMode_texture3(vec4 c, vec2 uv);

void main() {

	vec4 c = vC;

	c = textureMode_texture0(c, vT);
	// c = textureMode_texture1(c, vT);
	// c = textureMode_texture2(c, vT);
	// c = textureMode_texture3(c, vT);


	// Apply fade - 

	float a = 

	fragColour = c;
}
