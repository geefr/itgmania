#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;
flat in int renderInstance;

const int MaxLights = 4;
const int MaxTextures = 4;
// 4 x 4 = 16 texture units -> The minimum number of samplers we're allowed under OpenGL
const int MaxRenderInstances = 4;

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

out vec4 fragColour;

// RageTypes TextureMode
const int TEXMODE_MODULATE = 0;
const int TEXMODE_GLOW = 1;
const int TEXMODE_ADD = 2;

vec4 blendTexture(vec4 cp, int i, vec2 uv)
{
	if( TextureSettings.t[(renderInstance * MaxRenderInstances) + i].enabled == 0 )
	{
		return cp;
	}

	vec4 cs = texture(Texture[(renderInstance * MaxRenderInstances) + i], uv);

	// TODO: Ideally this would be preprocessor, but there's 4 texture units to handle?
	// TODO: These modes are sufficient for the stepmania renderer, would be nice to
	//       support more though?
	if( TextureSettings.t[(renderInstance * MaxRenderInstances) + i].envMode == TEXMODE_MODULATE )
	{
		if( Matrices.m[renderInstance].enableLighting != 0)
		{
			// TODO: This should be "if texture format == rgb"
			cp.rgb *= cs.rgb;
		}
		else
		{
			cp *= cs;
		}
	}
	else if( TextureSettings.t[(renderInstance * MaxRenderInstances) + i].envMode == TEXMODE_GLOW )
	{
		// GL_COMBINE_RGB: GL_REPLACE
		// GL_SOURCE0_RGB: GL_PRIMARY_COLOR
		vec4 c = vec4(cp.rgb, 1.0);

		// GL_COMBINE_ALPHA: GL_MODULATE
		// GL_OPERAND0_ALPHA: GL_SRC_ALPHA
		// GL_SOURCE0_ALPHA: GL_PRIMARY_COLOR
		// GL_OPERAND1_ALPHA: GL_SRC_ALPHA
		// GL_SOURCE1_ALPHA: GL_TEXTURE
		c.a = cp.a * cs.a;
		cp = c;
	}
	else if( TextureSettings.t[(renderInstance * MaxRenderInstances) + i].envMode == TEXMODE_ADD )
	{
		cp.rgb += cs.rgb;
		cp.a *= cs.a;
	}

	cp = clamp(cp, 0.0, 1.0);
	return cp;
}

void dodgyPhong(int i, vec3 viewDir, vec3 n, inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
	if( Lights.l[(renderInstance * MaxRenderInstances) + i].enabled == 0 )
	{
		return;
	}

	vec4 lambient = Lights.l[(renderInstance * MaxRenderInstances) + i].ambient;
	// vec4 lambient = vec4(0.1, 0.1, 0.1, 1.0);
	vec4 ldiffuse = Lights.l[(renderInstance * MaxRenderInstances) + i].diffuse;
	// vec4 ldiffuse = vec4(0.4, 0.4, 0.4, 1.0);
	vec4 lspecular = Lights.l[(renderInstance * MaxRenderInstances) + i].specular;
	// vec4 lspecular = vec4(1.0, 0.8, 0.2, 1.0);
	vec4 lposition = Lights.l[(renderInstance * MaxRenderInstances) + i].position;
	// vec4 lposition = vec4(0.0, 20.0, 10.0, 1.0);
	float mshininess = min(Material.mat[renderInstance].shininess, 1.0);
	// float mshininess = 3.0;

	vec3 ldirection;
	if( lposition.w == 0.0 )
	{
		ldirection = normalize( - (Matrices.m[renderInstance].modelView * lposition)).xyz;
		// ldirection = normalize(- lposition).xyz;
	}
	else
	{
		ldirection = normalize( - vP - (Matrices.m[renderInstance].modelView * lposition) ).xyz;
		// ldirection = normalize(- vP - lposition).xyz;
	}

	vec3 eyeDir = normalize(- vP).xyz;
	ambient += lambient;
	float ndotv = max(0.0, dot(n, ldirection));
	if( ndotv > 0.0 )
	{
		diffuse += ldiffuse * ndotv;
	}
	float pf = max(0.0, dot(eyeDir, reflect(ldirection, n)));
	if( pf > 0.0 )
	{
		pf = pow(pf, mshininess);
		specular += lspecular * pf;
	}
}

void main() {
	vec3 viewDir = normalize(-vP.xyz);
	vec3 n = normalize(vN);

	vec4 c = vec4(0.0, 0.0, 0.0, 0.0);
	bool readTexture0 = false;
	if( Matrices.m[renderInstance].enableVertexColour != 0 )
	{
		c = vC;

		if( Matrices.m[renderInstance].enableLighting != 0 )
		{
			// This case should never happen!
			fragColour = vec4(1.0, 0.0, 1.0, 1.0);
			return;
		}
	}
	else
	{
		if( Matrices.m[renderInstance].enableLighting != 0 )
		{
			// TODO: Probably not right at all
			c = vec4(1.0);
		}
		else
		{
			c = Material.mat[renderInstance].diffuse;

			c.rgb += Material.mat[renderInstance].emissive.rgb;
			// TODO: This was the logic in RageDisplay_Legacy,
			//       but including ambient makes mines overly bright
			//       on the cel noteskin (only!?).
			//       Excluding ambient however makes all arrows
			//       quite dim.
			c.rgb += Material.mat[renderInstance].ambient.rgb;
			c.a = 1.0;
		}
	}

	if( Matrices.m[renderInstance].enableLighting != 0 )
	{
		vec4 ambient = vec4(0.0);
		vec4 diffuse = vec4(0.0);
		vec4 specular = vec4(0.0);

		dodgyPhong(0, viewDir, n, ambient, diffuse, specular);
		dodgyPhong(1, viewDir, n, ambient, diffuse, specular);
		dodgyPhong(2, viewDir, n, ambient, diffuse, specular);
		dodgyPhong(3, viewDir, n, ambient, diffuse, specular);

		ambient = clamp(ambient, 0.0, 1.0);
		diffuse = clamp(diffuse, 0.0, 1.0);
		specular = clamp(specular, 0.0, 1.0);

		c *= Material.mat[renderInstance].emissive +
		    (Material.mat[renderInstance].ambient * ambient) +
		    (Material.mat[renderInstance].diffuse * diffuse) +
			(Material.mat[renderInstance].specular * specular);
		c.a = Material.mat[renderInstance].diffuse.a;
	}

	c = blendTexture(c, 0, vT);
	c = blendTexture(c, 1, vT);
	c = blendTexture(c, 2, vT);
	c = blendTexture(c, 3, vT);

	if( Matrices.m[renderInstance].enableAlphaTest != 0 )
	{
		if( c.a <= Matrices.m[renderInstance].alphaTestThreshold )
		{
			discard;
		}
	}

	fragColour = c;
}
