#version 330 
#extension GL_ARB_explicit_uniform_location : require
#extension GL_ARB_shading_language_420pack : require

layout(location = 0)  in      float height;

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
    float earth_radius = 637800.0;

    float x = (gl_VertexID % 1201) / 1200.0;
    float y = (gl_VertexID / 1201) / 1200.0;

    x = (x + longitude_degrees);
    y = (y +  latitude_degrees);

    float longitude_radians = radians(x);
    float  latitude_radians = radians(y);

    float radius = earth_radius + height / 10.0;
    vec3 position = vec3(
        cos(latitude_radians) * cos(longitude_radians),
        sin(latitude_radians),
        cos(latitude_radians) * sin(longitude_radians)
    );

    position *= radius;

    // Przekazanie koloru
    fragColor = heightToColor(height);

    // Przekształcenie w przestrzeni świata do przestrzeni widoku/projekcji
    gl_Position = projection * view * vec4(position, 1.0);
}
