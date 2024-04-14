#version 130

/*
 * Shader for sprite rendering
 * TODO: Description of what a sprite is/needs/supports
 * - Sprites do not need the per-vertex textureMatrixScale, though it may need to be part of a common vertex definition
 * - Sprites do need per-draw textureMatrixScale, that's used for arrow rendering and similar
 */
const int MaxTextures = 4;

in vec3 vPin;
in vec3 vNin;
in vec4 vCin;
in vec2 vTin;

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 textureMat;
uniform bool enableTextureMatrixScale;
uniform vec2 textureMatrixScale;
uniform bool texture0Enabled;

uniform sampler2D texture0;

out vec4 vP;
out vec3 vN;
out vec4 vC;
out vec2 vT;

float det(mat2 matrix) {
    return matrix[0].x * matrix[1].y - matrix[0].y * matrix[1].x;
}

mat3 inverse(mat3 matrix) {
    vec3 row0 = matrix[0];
    vec3 row1 = matrix[1];
    vec3 row2 = matrix[2];

    vec3 minors0 = vec3(
        det(mat2(row1.y, row1.z, row2.y, row2.z)),
        det(mat2(row1.z, row1.x, row2.z, row2.x)),
        det(mat2(row1.x, row1.y, row2.x, row2.y))
    );
    vec3 minors1 = vec3(
        det(mat2(row2.y, row2.z, row0.y, row0.z)),
        det(mat2(row2.z, row2.x, row0.z, row0.x)),
        det(mat2(row2.x, row2.y, row0.x, row0.y))
    );
    vec3 minors2 = vec3(
        det(mat2(row0.y, row0.z, row1.y, row1.z)),
        det(mat2(row0.z, row0.x, row1.z, row1.x)),
        det(mat2(row0.x, row0.y, row1.x, row1.y))
    );

    mat3 adj = transpose(mat3(minors0, minors1, minors2));

    return (1.0 / dot(row0, minors0)) * adj;
}

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
}
