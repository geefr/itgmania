#version 400 core

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec3 vNin;
layout (location = 2) in vec4 vCin;
layout (location = 3) in vec2 vTin;
layout (location = 4) in vec2 textureMatrixScaleIn;

// All rage matrices are 4x4, even the texture one
uniform mat4 projectionMatrix;
uniform mat4 modelViewMatrix;
uniform mat4 textureMatrix;

uniform int vertexColourEnabled;
uniform int textureMatrixScaleEnabled;

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

	if( vertexColourEnabled != 0)
	{
		vC = vCin;
	}
	else
	{
		vC = vec4(1.0, 1.0, 1.0, 1.0);
	}

	if( textureMatrixScaleEnabled != 0)
	{
		// TODO: Took this from Texture matrix scaling.vert
		//       I don't understand it :[
		vT = texMatrix * vTin;
		vT = vT * (vec2(1.0) - vT);
	}
	else
	{
		vT = texMatrix * vTin;
	}
}
