#version 300 es

precision mediump float;

uniform vec2 u_resolution;
uniform vec2 u_mouse;
uniform float u_time;
uniform sampler2D u_buffer0;

out vec4 fragColour;

// left, bottom, top, right
vec4 fadeSize = vec4(0.2, 0.1, 0.1, 0.2);
vec4 cropSize = vec4(0.3, 0.2, 0.0, 0.0);

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

void main() {
    // fadeSize = vec4(
    //     max(0.3, cos(u_time * 8.1) * 0.75 ),
    //     max(0.3, cos(u_time * 3.0) * 0.75 ),
    //     max(0.3, cos(u_time * 4.5) * 0.75 ),
    //     max(0.3, cos(u_time * 7.3) * 0.75 )
    // );

    vec2 vB = gl_FragCoord.xy / u_resolution.xy;
    vec4 colour = texture(u_buffer0, vB);
    vB.y = 1.0 - vB.y;

    // Better version
    float d = fadecrop(vB);
    if( d > 1.0 ) {
        discard;
    }

    vec2 st = gl_FragCoord.xy / u_resolution.xy;
    st.x *= u_resolution.x / u_resolution.y;

    float a = 1.0 - d;
    fragColour = vec4(colour.rgb, a);
}

