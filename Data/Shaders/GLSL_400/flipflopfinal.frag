#version 400 core

in vec4 vP;
in vec2 vT;

uniform sampler2D texPreviousFrame;

out vec4 fragColour;

void main() {
    fragColour = texture(texPreviousFrame, vT);
}
