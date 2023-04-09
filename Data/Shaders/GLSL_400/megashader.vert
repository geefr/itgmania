#version 400 core

const int MaxLights = 4;
const int MaxTextures = 4;

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec3 vNin;
layout (location = 2) in vec4 vCin;
layout (location = 3) in vec2 vTin;
layout (location = 4) in vec2 textureMatrixScaleIn;

layout (std140) uniform MatricesBlock {
	mat4 modelView;
	mat4 projection;
	mat4 texture;

	int enableAlphaTest;
	int enableLighting;
	int enableVertexColour;
	int enableTextureMatrixScale;

	float alphaTestThreshold;
	float pad1;
	float pad2;
	float pad3;
} Matrices;

struct TextureSetting {
	int enabled;
	int envMode;
	int pad4;
	int pad5;
};
layout (std140) uniform TextureSettingsBlock {
	TextureSetting t[MaxTextures];
} TextureSettings;
uniform sampler2D Texture[MaxTextures];

layout (std140) uniform MaterialBlock {
	vec4 emissive;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
	float pad6;
	float pad7;
	float pad8;
} Material;

struct Light {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
	int enabled;
	int pad9;
	int pad10;
	int pad11;
};
layout (std140) uniform LightsBlock {
	Light l[MaxLights];
} Lights;

out vec4 vP;
out vec3 vN;
out vec4 vC;
out vec2 vT;

void main()
{
	mat4 mvp = Matrices.projection * Matrices.modelView;
	mat3 normalMatrix = transpose(inverse(mat3(Matrices.modelView)));

	gl_Position = mvp * vec4(vPin, 1.0);

	vP = Matrices.modelView * vec4(vPin, 1.0);
	vN = normalize(normalMatrix * vNin);

	vC = vCin;
	
	// texture matrix scaling is used to control animation. It's either zero or one, never between.
	// So scaling == 0 removes any translation from the texture matrix, for a given axis
	// It's also the same for all vertices (observed), so may be fairly wasteful
	// texture matrix is identity, plus translation (last column)
	// texture matrix is a mat4 like all Rage matrices
	// See 'Texture matrix scaling.vert' for old version, I removed the magic
	// scaling function and went with an if statement, because I'm simple like that.
	
	mat4 texMatrix = Matrices.texture;
	if( Matrices.enableTextureMatrixScale != 0 )
	{
		texMatrix[3][0] *= textureMatrixScaleIn.x;
		texMatrix[3][1] *= textureMatrixScaleIn.y;
	}
	vec4 vT4 = vec4(vTin, 0.0, 1.0);
	vT = vec2(texMatrix * vT4);
}
