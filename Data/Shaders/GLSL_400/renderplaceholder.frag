#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;

uniform sampler2D tex0;
uniform vec4 materialEmissive;
uniform vec4 materialAmbient;
uniform vec4 materialDiffuse;
uniform vec4 materialSpecular;
uniform float materialShininess;

out vec4 fragColour;

const vec4 shadowColor = vec4(0.6, 0.75, 0.9, 1.0);

void main() {
	vec3 nor, light;
	nor = normalize(vN);
	
	// vec4 diffuse, specular, color, lightSource;
	// float ambient = length(gl_FrontMaterial.ambient.rgb);
	// lightSource = gl_LightSource[0].position;
	// light = normalize(gl_ModelViewMatrix * lightSource).xyz;
	
	vec4 texColour = texture2D(tex0, vT);

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

	if( texColour.a < 0.01 )
	{
		discard;
	}

	fragColour = vec4(vC.rgb + texColour.rgb, texColour.a);

	// fragColour = texColour;
	// fragColour = vec4(vC.rgb, 1.0);
}
