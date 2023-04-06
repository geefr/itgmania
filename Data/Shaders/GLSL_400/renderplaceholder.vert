#version 400 core

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec3 vNin;
layout (location = 2) in vec4 vCin;
layout (location = 3) in vec2 vTin;

out vec3 vP;
out vec3 vN;
out vec4 vC;
out vec2 vT;

void main()
{
	// TODO: Matrices and all that
	gl_Position = vec4(vPin, 1.0);

	vP = vPin;
	vN = vNin;
	vC = vCin;
	vT = vTin;
}
