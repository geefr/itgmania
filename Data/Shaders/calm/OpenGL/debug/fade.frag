#version 300 es

precision mediump float;

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
uniform sampler2D u_buffer0;

out vec4 fragColour;

// left, bottom, top, right
vec4 fadeCoords = vec4(0.2, 0.2, 0.2, 0.2);


float insideBox(vec2 v, vec2 tl, vec2 br) {
    vec2 s = step(tl, v) - step(br, v);
    return s.x * s.y;
}

float fade(vec2 vB) {
    vec2 tl = fadeCoords.xz;
    vec2 tr = vec2(1.0 - fadeCoords.w, fadeCoords.z);
    vec2 bl = vec2(fadeCoords.x, 1.0 - fadeCoords.y);
    vec2 br = vec2(1.0) - fadeCoords.wy;

    float inside = insideBox(vB, tl, br);
    float inFade = 1.0 - inside;

    float isLeft = 1.0 - step(tl.x, vB.x);
    float isBottom = step(br.y, vB.y);
    float isUp = 1.0 - step(tl.y, vB.y);
    float isRight = step(br.x, vB.x);

    // Distance from inner edge of fade region, 0.0 -> 1.0
    float d = 0.0;
    if( tl.x > 0.0 ) {
        d += isLeft * mix(0.0, 1.0, (abs(vB.x - tl.x) / tl.x));
    }
    if( tl.y > 0.0 ) {
        d += isUp * mix(0.0, 1.0, (abs(vB.y - tl.y) / tl.y));
    }
    if( br.y < 1.0 ) {
        d += isBottom * mix(0.0, 1.0, (abs(vB.y - br.y) / (1.0 - br.y)));
    }
    if( br.x < 1.0 ) {
        d += isRight * mix(0.0, 1.0, (abs(vB.x - br.x) / (1.0 - br.x)));
    }

    return d;
}

void main() {
    fadeCoords = vec4(
        max(0.3, cos(u_time * 8.1) * 0.75 ),
        max(0.3, cos(u_time * 3.0) * 0.75 ),
        max(0.3, cos(u_time * 4.5) * 0.75 ),
        max(0.3, cos(u_time * 7.3) * 0.75 )
    );

    vec2 vB = gl_FragCoord.xy / u_resolution.xy;
    vec4 colour = texture(u_buffer0, vB);
    vB.y = 1.0 - vB.y;

    vec2 st = gl_FragCoord.xy / u_resolution.xy;
    st.x *= u_resolution.x / u_resolution.y;

    // Better version
    float d = fade(vB);
    float a = 1.0 - d;
    fragColour = vec4(colour.rgb, a);
}

