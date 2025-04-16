#version 430 core

layout(location = 0) in vec3 position;
out vec3 fragColor; // Output color to fragment shader

void main() {
    gl_Position = vec4(position, 1.0);
}