#version 400 core

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec3 vNin;
layout (location = 2) in vec4 vCin;
layout (location = 3) in vec2 vTin;

// All rage matrices are 4x4, even the texture one
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;

out vec4 vP;
out vec3 vN;
out vec4 vC;
out vec2 vT;

void main()
{
	mat4 mvp = projectionMatrix * modelViewMatrix;
	mat3 normalMatrix = transpose(inverse(mat3(modelViewMatrix)));
	mat2 texMatrix = mat2(textureMatrix);

	gl_Position = mvp * vec4(vPin, 1.0);

	vP = modelViewMatrix * vec4(vPin, 1.0);
	vN = normalize(normalMatrix * vNin);
	vC = vCin;
	vT = texMatrix * vTin;
}
