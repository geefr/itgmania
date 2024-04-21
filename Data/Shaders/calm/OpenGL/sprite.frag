#version 400 core

in vec4 vP;
in vec3 vN;
in vec4 vC;
in vec2 vT;
in vec2 vB;

const int MaxTextures = 4;

uniform mat4 modelViewMat;
uniform mat4 projectionMat;
uniform mat4 textureMat;
uniform bool enableTextureMatrixScale;
uniform vec2 textureMatrixScale;
uniform bool texture0Enabled;

// left, bottom, top, right
// Fraction of sprite (vP)
uniform vec4 fadeSize;
uniform vec4 cropSize;

uniform sampler2D texture0;

out vec4 fragColour;

float fadecrop(vec2 vB) {
    vec2 tl = fadeSize.xz + cropSize.xz;
    vec2 br = vec2(1.0) - fadeSize.wy - cropSize.wy;

    float isLeft = 1.0 - step(tl.x, vB.x);
    float isBottom = step(br.y, vB.y);
    float isUp = 1.0 - step(tl.y, vB.y);
    float isRight = step(br.x, vB.x);

    // Distance from inner edge of fade region to inner edge of crop region, 0.0 -> 1.0
    float d = 0.0;
    d += isLeft * mix(0.0, 1.0, (abs(vB.x - tl.x) / max(tl.x - cropSize.x, 0.00001)));
    d += isUp * mix(0.0, 1.0, (abs(vB.y - tl.y) / max(tl.y - cropSize.z, 0.00001)));
    d += isBottom * mix(0.0, 1.0, (abs(vB.y - br.y) / max(1.0 - br.y - cropSize.y, 0.00001)));
    d += isRight * mix(0.0, 1.0, (abs(vB.x - br.x) / max(1.0 - br.x - cropSize.w, 0.00001)));

    return d;
}

vec4 textureMode_texture0(vec4 c, vec2 uv);
// vec4 textureMode_texture1(vec4 c, vec2 uv);
// vec4 textureMode_texture2(vec4 c, vec2 uv);
// vec4 textureMode_texture3(vec4 c, vec2 uv);

void main() {
    // Calculate crop/fade early
    float d = fadecrop(vB);
    if( d > 1.0 ) {
        discard;
    }

    // Blend vertex colour and textures
	vec4 c = vC;
	c = textureMode_texture0(c, vT);
	// c = textureMode_texture1(c, vT);
	// c = textureMode_texture2(c, vT);
	// c = textureMode_texture3(c, vT);


	// Apply fade
    c.a *= 1.0 - d;
	fragColour = c;
}
