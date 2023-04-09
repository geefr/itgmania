#version 400 core

in vec4 vP;
in vec2 vT;

uniform sampler2D texPreviousFrame;

out vec4 fragColour;

void main() {
    // TODO: Could do all sorts of evil things here >:3
    fragColour = texture(texPreviousFrame, vT);
}
