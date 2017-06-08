#version 330

in vec2 fragmentTexCoord;
//in vec4 l;
//in vec4 n;
out vec4 outColor;

uniform sampler2D Texture;

void main() {
    outColor = texture(Texture, fragmentTexCoord);
/*
    const vec4 diffColor = vec4(0.2f, 0.2f, 0.2f, 1.0f);
    vec4 n1 = normalize(n);
    vec4 l1 = normalize(l);

    vec4 color = texture2D(Texture, fragmentTexCoord);
    outColor = clamp(color + diffColor * max(dot(l1,n1), 0.0f), 0.0, 1.0);
*/
}
