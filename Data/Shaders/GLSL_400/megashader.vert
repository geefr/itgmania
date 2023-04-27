#version 400 core

const int MaxLights = 4;
const int MaxTextures = 4;
// 4 x 4 = 16 texture units -> The minimum number of samplers we're allowed under OpenGL
const int MaxRenderInstances = 4;

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec3 vNin;
layout (location = 2) in vec4 vCin;
layout (location = 3) in vec2 vTin;
layout (location = 4) in vec2 textureMatrixScaleIn;

// This is like instancing, but we can't actually use instancing,
// since we're batching multiple different draws together.
// This is a tad wasteful, but fast.
layout (location = 5) in int renderInstanceIn;

struct MatrixSetting {
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
};

layout (std140) uniform MatricesBlock {
	MatrixSetting m[MaxRenderInstances];
} Matrices;

struct TextureSetting {
	int enabled;
	int envMode;
	int pad4;
	int pad5;
};
layout (std140) uniform TextureSettingsBlock {
	TextureSetting t[MaxTextures * MaxRenderInstances];
} TextureSettings;
uniform sampler2D Texture[MaxTextures * MaxRenderInstances];

struct MaterialSetting {
	vec4 emissive;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
	float pad6;
	float pad7;
	float pad8;
};

layout (std140) uniform MaterialBlock {
	MaterialSetting mat[MaxRenderInstances];
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
	Light l[MaxLights * MaxRenderInstances];
} Lights;

out vec4 vP;
out vec3 vN;
out vec4 vC;
out vec2 vT;
flat out int renderInstance;

void main()
{
	mat4 pMat = Matrices.m[renderInstanceIn].projection;
	mat4 mvMat = Matrices.m[renderInstanceIn].modelView;

	mat4 mvp = pMat * mvMat;
	mat3 normalMatrix = transpose(inverse(mat3(mvMat)));

	gl_Position = mvp * vec4(vPin, 1.0);

	vP = mvMat * vec4(vPin, 1.0);
	vN = normalize(normalMatrix * vNin);

	vC = vCin;
	
	// texture matrix scaling is used to control animation. It's either zero or one, never between.
	// So scaling == 0 removes any translation from the texture matrix, for a given axis
	// It's also the same for all vertices (observed), so may be fairly wasteful
	// texture matrix is identity, plus translation (last column)
	// texture matrix is a mat4 like all Rage matrices
	// See 'Texture matrix scaling.vert' for old version, I removed the magic
	// scaling function and went with an if statement, because I'm simple like that.
	
	mat4 texMatrix = Matrices.m[renderInstanceIn].texture;
	if( Matrices.m[renderInstanceIn].enableTextureMatrixScale != 0 )
	{
		texMatrix[3][0] *= textureMatrixScaleIn.x;
		texMatrix[3][1] *= textureMatrixScaleIn.y;
	}
	vec4 vT4 = vec4(vTin, 0.0, 1.0);
	vT = vec2(texMatrix * vT4);

	renderInstance = renderInstanceIn;
}
