#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;

uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;

uniform sampler2D tex0;

uniform vec4 materialEmissive;
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;
uniform float materialShininess;

uniform int texModeModulate;
uniform int texModeAdd;
uniform int texModeGlow;

out vec4 fragColour;

const vec4 shadowColor = vec4(0.6, 0.75, 0.9, 1.0);

void main() {
	vec3 nor, light;
	nor = normalize(vN);
	
	// vec4 diffuse, specular, color, lightSource;
	// float ambient = length(gl_FrontMaterial.ambient.rgb);
	// lightSource = gl_LightSource[0].position;
	// light = normalize(gl_ModelViewMatrix * lightSource).xyz;
	
	vec4 texColour = texture(tex0, vT);

	// float intensity = max(dot(light,nor), 0.0);
	// if (intensity < 0.5) {
	// 	intensity *= 0.5;
	// 	color *= shadowColor;
	// }
	// intensity = min(clamp(intensity, ambient, 1.0) + 0.25, 1.0);
	
	// TODO: In the old shaders it says "Don't allow transparency. Bad Things will happen."
	//       On the contrary we do want alpha here - But also will need to handle the
	//       various modes in RageDisplay::SetAlphaTest

	// TODO: At least for DrawQuadsInternal, textures are used as masks, and multiplied by the vertex colour
	//       But alpha comes purely from the texture (maybe depending on the alpha mode stuff mentioned above)
	// TODO: And there's some very strange behaviour here, perhaps it's different between calls to DrawQuadsInternal
	//       by other stuff being modified!?


	// TODO: Nasty if statement - Should be preprocessed :)
	// See texture mode definition at https://docs.gl/gl3/glTexEnv
	if( texModeAdd != 0 )
	{
		// Placeholder - Make add solid
		fragColour = vec4(texColour.rgb, 1.0);
	}
	else if( texModeModulate != 0 )
	{
		// Placeholder - Make modulate alpha
		// fragColour = vec4(0.6, 0.2, 0.6, texColour.a);
		fragColour = texColour * vC;
		if( texColour.a > 0.01 )
		{
		  fragColour = max(fragColour, vec4(0.1, 0.1, 0.1, 0.3));
		}
	}
	else if( texModeGlow != 0 )
	{
		// Placeholder - Make glow just white
		fragColour = vec4(1.0, 1.0, 1.0, texColour.a);
	}
	else
	{
		fragColour = vec4(vC.rgb, 1.0);
		// fragColour = texColour;
	}
}
