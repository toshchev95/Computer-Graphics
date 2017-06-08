#version 330 core

in vec2 TexCoord;

out vec4 outColor;

uniform sampler2D Texture;

void main()
{
    outColor = texture2D(Texture, TexCoord);
}
