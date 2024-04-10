#version 400 core

layout (location = 0) in vec3 vPin;
layout (location = 1) in vec2 vTin;

out vec4 vP;
out vec2 vT;

void main()
{
    gl_Position = vec4(vPin, 1.0);
    vP = vec4(vPin, 1.0);
    vT = vTin;
}
