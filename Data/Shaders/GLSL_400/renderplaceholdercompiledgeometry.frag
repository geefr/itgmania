#version 400 core

// Interface constants
const int numTextures = 4;
const int numLights = 4;

// RageTypes TextureMode
const int TEXMODE_MODULATE = 0;
const int TEXMODE_GLOW = 1;
const int TEXMODE_ADD = 2;

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;

struct Texture {
  sampler2D tex;
  bool enabled;
  int envMode;
};
uniform Texture tex[numTextures];

uniform sampler2D texPreviousFrame;

struct Material {
	vec4 emissive;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};
uniform Material material;

struct Light {
	bool enabled;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;
};
uniform Light lights[numLights];

uniform vec2 renderResolution;

uniform bool alphaTestEnabled;
uniform float alphaTestThreshold;

// Typical / observed behaviour
// * Lighting is enabled for character models
// * Lighting is disabled for other compiled geometry (noteskins)
// * Vertex colour is enabled for quads, menu elements, backgrounds, hold tails, etc
// * Vertex colour is disabled for compiled geometry (noteskins and characters)
// * Texture matrix scale is enabled, if texture animations are disabled on a per-vertex basis (Arrow border on Cel and similar)
// * Texture matrix scale is disabled for everything else (Arrow center on Cel and characters)
uniform bool lightingEnabled;
uniform bool vertexColourEnabled;
uniform bool textureMatrixScaleEnabled;

out vec4 fragColour;

vec4 blendTexture(vec4 cp, int i, vec2 uv)
{
	if( ! tex[i].enabled )
	{
		return cp;
	}

	vec4 cs = texture(tex[i].tex, uv);

	// TODO: Ideally this would be preprocessor, but there's 4 texture units to handle?
	// TODO: These modes are sufficient for the stepmania renderer, would be nice to
	//       support more though?
	if( tex[i].envMode == TEXMODE_MODULATE )
	{
		if( lightingEnabled )
		{
			// TODO: This should be "if texture format == rgb"
			cp.rgb *= cs.rgb;
		}
		else
		{
			cp *= cs;
		}
	}
	else if( tex[i].envMode == TEXMODE_GLOW )
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
	else if( tex[i].envMode == TEXMODE_ADD )
	{
		cp.rgb += cs.rgb;
		cp.a *= cs.a;
	}

	cp = clamp(cp, 0.0, 1.0);
	return cp;
}

void dodgyPhong(int i, vec3 viewDir, vec3 n, inout vec4 ambient, inout vec4 diffuse, inout vec4 specular)
{
	if( !lights[i].enabled )
	{
		return;
	}

	vec4 lambient = lights[i].ambient;
	// vec4 lambient = vec4(0.1, 0.1, 0.1, 1.0);
	vec4 ldiffuse = lights[i].diffuse;
	// vec4 ldiffuse = vec4(0.4, 0.4, 0.4, 1.0);
	vec4 lspecular = lights[i].specular;
	// vec4 lspecular = vec4(1.0, 0.8, 0.2, 1.0);
	vec3 ldirection;

	vec4 lposition = lights[i].position;
	// vec4 lposition = vec4(0.0, 80.0, 10.0, 1.0);
	if( lposition.w == 0.0 )
	{
		ldirection = normalize( (modelViewMatrix * lposition)).xyz;
		// ldirection = normalize(lposition).xyz;
	}
	else
	{
		ldirection = normalize( vP - (modelViewMatrix * lposition) ).xyz;
		// ldirection = normalize(vP - lposition).xyz;
	}

	float mshininess = min(material.shininess, 1.0);
	// float mshininess = 25.0;

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
	vec2 fragPosT = (gl_FragCoord.xy / renderResolution);
	vec4 fragColourPrev = texture(texPreviousFrame, fragPosT);
	vec3 viewDir = normalize(-vP.xyz);
	vec3 n = normalize(vN);

	vec4 c = vec4(0.0, 0.0, 0.0, 0.0);
	bool readTexture0 = false;
	if( vertexColourEnabled )
	{
		c = vC;
	}
	else
	{
		if( lightingEnabled )
		{
			// TODO: Probably not right at all
			c = material.emissive;
		}
		else
		{
			c = material.diffuse;
			
			c.rgb += material.emissive.rgb;
			// TODO: This was the logic in RageDisplay_Legacy,
			//       but including ambient makes mines overly bright
			//       on the cel noteskin (only!?).
			//       Excluding ambient however makes all arrows
			//       quite dim.
			c.rgb += material.ambient.rgb;
			c.a = 1.0;
		}
	}

	if( lightingEnabled )
	{
		vec4 ambient = vec4(0.0);
		vec4 diffuse = vec4(0.0);
		vec4 specular = vec4(0.0);

		dodgyPhong(0, viewDir, n, ambient, diffuse, specular);
		dodgyPhong(1, viewDir, n, ambient, diffuse, specular);
		dodgyPhong(2, viewDir, n, ambient, diffuse, specular);
		dodgyPhong(3, viewDir, n, ambient, diffuse, specular);

		c *= (material.ambient * ambient) +
		     (material.diffuse * diffuse) +
			 (material.specular * specular);
	}

	c = blendTexture(c, 0, vT);
	c = blendTexture(c, 1, vT);
	c = blendTexture(c, 2, vT);
	c = blendTexture(c, 3, vT);

	if( alphaTestEnabled )
	{
		if( c.a <= alphaTestThreshold )
		{
			discard;
		}
	}

	fragColour = c;
}
