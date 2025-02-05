#version 330 
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_shading_language_420pack : require

in vec3 fragColor;
out vec4 color;

void main() {
    color = vec4(fragColor, 1.0);
}