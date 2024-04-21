#version 400 core

/*
 * Shader for sprite rendering
 * TODO: Description of what a sprite is/needs/supports
 * - Sprites do not need the per-vertex textureMatrixScale, though it may need to be part of a common vertex definition
 * - Sprites do need per-draw textureMatrixScale, that's used for arrow rendering and similar
 */
const int MaxTextures = 4;

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec3 vNin;
layout (location = 2) in vec4 vCin;
layout (location = 3) in vec2 vTin;
layout (location = 4) in vec2 vBin;

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 textureMat;
uniform bool enableTextureMatrixScale;
uniform vec2 textureMatrixScale;
uniform bool texture0Enabled;
uniform vec4 fadeCoords; // left, bottom, top, right

uniform sampler2D texture0;

out vec4 vP;
out vec3 vN;
out vec4 vC;
out vec2 vT;
out vec2 vB;

void main()
{
	mat4 mvp = projectionMat * modelViewMat;
	mat3 normalMatrix = transpose(inverse(mat3(modelViewMat)));

	gl_Position = mvp * vec4(vPin, 1.0);

	vP = modelViewMat * vec4(vPin, 1.0);
	vN = normalize(normalMatrix * vNin);
	vC = vCin;
	
	// TODO CALM: Verify this stuff as needed - Split the shaders between sprites and models, since each needs to be optimised for their own stuff
	// texture matrix scaling is used to control animation. It's either zero or one, never between.
	// So scaling == 0 removes any translation from the texture matrix, for a given axis
	// It's also the same for all vertices (observed), so may be fairly wasteful
	// texture matrix is identity, plus translation (last column)
	// texture matrix is a mat4 like all Rage matrices
	// See 'Texture matrix scaling.vert' for old version, I removed the magic
	// scaling function and went with an if statement, because I'm simple like that.

	mat4 tMat = textureMat;
	if( enableTextureMatrixScale )
	{
		tMat[3][0] *= textureMatrixScale.x;
		tMat[3][1] *= textureMatrixScale.y;
	}
	vec4 vT4 = vec4(vTin, 0.0, 1.0);
	vT = vec2(tMat * vT4);

	vB = vBin;
}
