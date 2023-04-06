#version 400 core

in vec3 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;

uniform sampler2D tex0;

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
	
	// Don't allow transparency. Bad Things will happen.
	fragColour = vec4(texColour.rgb, 1.0);
}
