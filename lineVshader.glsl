#version 430 core
layout(location = 0) in vec3 aPos;
uniform vec2 lineOffset;
void main()
{
    gl_Position = vec4(aPos.xy + lineOffset, 0.0, 1.0);
}