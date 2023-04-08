#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;

const int numTextures = 4;
uniform sampler2D tex[numTextures];
uniform int texEnabled[numTextures];
uniform int texEnvMode[numTextures];

uniform sampler2D texPreviousFrame;

uniform vec4 materialEmissive;
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;
uniform float materialShininess;

uniform vec2 renderResolution;

uniform int alphaTestEnabled;
uniform float alphaTestThreshold;

uniform int vertexColourEnabled;
uniform int textureMatrixScaleEnabled;

out vec4 fragColour;

const int GL_MODULATE = 0x2100;
const int GL_ADD = 0x0104;

const int TEXMODE_MODULATE = 0;
const int TEXMODE_GLOW = 1;
const int TEXMODE_ADD = 2;

const vec4 shadowColor = vec4(0.6, 0.75, 0.9, 1.0);

const vec3 ambient = vec3(1.0, 1.0, 1.0);
const vec3 diffuse = vec3(1.0, 1.0, 1.0);
const vec3 specular = vec3(1.0, 1.0, 1.0);

vec4 blendTexture(vec4 cp, int i, vec2 uv)
{
	if( texEnabled[i] == 0 )
	{
		return cp;
	}

	vec4 cs = texture(tex[i], uv);

	// TODO: Ideally this would be preprocessor, but there's 4 texture units to handle?
	// TODO: These modes are sufficient for the stepmania renderer, would be nice to
	//       support more though?
	if( texEnvMode[i] == TEXMODE_MODULATE )
	{
		cp *= cs;
	}
	else if( texEnvMode[i] == TEXMODE_GLOW )
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
	else if( texEnvMode[i] == TEXMODE_ADD )
	{
		cp.rgb += cs.rgb;
		cp.a *= cs.a;
	}

	cp = clamp(cp, 0.0, 1.0);
	return cp;
}

void main() {

	// if( vertexColourEnabled == 0 )
	// {
	// 	fragColour = vec4(1.0, 0.0, 1.0, 1.0);
	// 	return;
	// }

	vec2 fragPosT = (gl_FragCoord.xy / renderResolution);
	vec4 fragColourPrev = texture(texPreviousFrame, fragPosT);

	// Vertex colour to start with
	// then blend each texture - Imitation of fixed function pipeline)
	vec4 c = vC;
	if( vertexColourEnabled == 0 && texEnabled[0] != 0)
	{	
		// TODO: Just use material diffuse?
		//       Or will model shaders always be different than sprite shaders?
		c = texture(tex[0], vT);
	}
	else
	{
		c = blendTexture(c, 0, vT);
	}

	c = blendTexture(c, 1, vT);
	c = blendTexture(c, 2, vT);
	c = blendTexture(c, 3, vT);

	if( alphaTestEnabled != 0 )
	{
		if( c.a <= alphaTestThreshold )
		{
			discard;
		}
	}

	fragColour = c;
}
