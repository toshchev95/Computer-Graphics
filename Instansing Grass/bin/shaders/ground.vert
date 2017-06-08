#version 330 core
layout (location = 0) in vec3 groundPosition;
layout (location = 1) in vec2 groundTextureCoord;

out vec2 TexCoord;

in vec4 point;

uniform mat4 camera;

void main()
{
    gl_Position = camera * point;
    TexCoord = groundTextureCoord;
}
