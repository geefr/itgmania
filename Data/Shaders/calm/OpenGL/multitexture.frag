#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;
in vec2 vB;

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 textureMat;
uniform bool enableTextureMatrixScale;
uniform vec2 textureMatrixScale;

// Multitexture - All 4
##TEXTURE_0_UNIFORMS##
##TEXTURE_1_UNIFORMS##
##TEXTURE_2_UNIFORMS##
##TEXTURE_3_UNIFORMS##

out vec4 fragColour;

##TEXTURE_0_MODE##
##TEXTURE_1_MODE##
##TEXTURE_2_MODE##
##TEXTURE_3_MODE##
##EFFECT_MODE##

void main() {
    // Blend vertex colour and textures
	vec4 c = vC;
	c = textureMode_0(c, vT);
	c = textureMode_1(c, vT);
	c = textureMode_2(c, vT);
	c = textureMode_3(c, vT);

    // Apply effect
    c = effectMode(c, vT);

	fragColour = c;
}
