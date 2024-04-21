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
uniform vec4 fadeCoords; // left, bottom, top, right

uniform sampler2D texture0;

out vec4 fragColour;

float fade(vec2 vB) {
    vec2 tl = fadeCoords.xz;
    vec2 br = vec2(1.0) - fadeCoords.wy;

    float isLeft = 1.0 - step(tl.x, vB.x);
    float isBottom = step(br.y, vB.y);
    float isUp = 1.0 - step(tl.y, vB.y);
    float isRight = step(br.x, vB.x);

    // Distance from inner edge of fade region, 0.0 -> 1.0
    float d = 0.0;
    d += isLeft * mix(0.0, 1.0, (abs(vB.x - tl.x) / max(tl.x, 0.00001)));
    d += isUp * mix(0.0, 1.0, (abs(vB.y - tl.y) / max(tl.y, 0.00001)));
    d += isBottom * mix(0.0, 1.0, (abs(vB.y - br.y) / max(1.0 - br.y, 0.00001)));
    d += isRight * mix(0.0, 1.0, (abs(vB.x - br.x) / max(1.0 - br.x, 0.00001)));

    return d;
}

vec4 textureMode_texture0(vec4 c, vec2 uv) {
    if( texture0Enabled == false ) {
        return c;
    }
    vec4 t = texture(texture0, uv);
    vec4 fragColor = vec4(c.rgb, (c.a * t.a));
    return fragColor;
}

void main() {

	vec4 c = vC;

	c = textureMode_texture0(c, vT);
	// c = textureMode_texture1(c, vT);
	// c = textureMode_texture2(c, vT);
	// c = textureMode_texture3(c, vT);

	// Apply fade
	float a = 1.0 - fade(vB);
    c.a *= a;

	fragColour = c;
}
