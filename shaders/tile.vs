#version 330 
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_shading_language_420pack : require

layout(location = 0)  in      float height;

layout(location = 4)  uniform float x_condensation;
layout(location = 5)  uniform   int  latitude_degrees;
layout(location = 6)  uniform   int longitude_degrees;

layout(location = 14) uniform mat4 view;
layout(location = 15) uniform mat4 projection;

out vec3 fragColor;

vec3 heightToColor(float ht) {
    if (ht < 0.0) return vec3(0.0, 0.0, 1.0);
    else if (ht < 500.0) return vec3(0.0, ht / 500.0, 0.0);
    else if (ht < 1000.0) return vec3((ht / 500.0) - 1.0, 1.0, 0.0);
    else if (ht < 2000.0) return vec3(1.0, 2.0 - ht / 1000.0, 0.0);
    else return vec3(1.0, ht / 2000.0 - 1.0, ht / 2000.0 - 1.0);
}

void main() {
    float x = (gl_VertexID % 1201) / 1200.0;
    float y = (gl_VertexID / 1201) / 1200.0;

    x = (x + longitude_degrees) * x_condensation;
    y = (y +  latitude_degrees);

    fragColor = heightToColor(height);
    gl_Position = projection * view * vec4(x, y, 0.0, 1.0);
}