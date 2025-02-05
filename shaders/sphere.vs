#version 330 
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_shading_language_420pack : require

layout(location = 0)  in      vec3  pos;

layout(location = 10) uniform mat4 model;
layout(location = 14) uniform mat4 view;
layout(location = 15) uniform mat4 projection;

out vec3 vpos;

void main(void) {
    vpos = pos;
    gl_Position = projection * view * model * vec4(pos, 1.0);
}