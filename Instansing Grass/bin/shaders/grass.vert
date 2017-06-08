#version 330
// http://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

in vec4 point;
in vec2 position;
in vec4 variance;
in vec2 texCoord;
//in vec3 normal;

out vec2 fragmentTexCoord;
//out vec4 l;
//out vec4 n;

uniform mat4 camera;

// change
void main() {
    float rnd = rand(position);
    mat4 scaleMatrix = mat4(1.0);
    scaleMatrix[0][0] = 0.01;
    scaleMatrix[1][1] = gl_InstanceID / 350000.0 + 0.03;//noise1(gl_InstanceID) + 0.04;

    mat4 rotationMatrix = mat4(1.0);

    rotationMatrix[0][0] = cos(rnd);
    rotationMatrix[0][2] = sin(rnd);
    rotationMatrix[2][0] = -sin(rnd);
    rotationMatrix[2][2] = cos(rnd);

    mat4 positionMatrix = mat4(1.0);
    positionMatrix[3][0] = position.x;
    positionMatrix[3][2] = position.y;
    // change
	gl_Position = camera * (positionMatrix * rotationMatrix * scaleMatrix * point
                            + 0.17 * gl_VertexID * variance * point.y);
    fragmentTexCoord = texCoord;
/*
    l = vec4(1.0f, 1.0f, 0.0f, 1.0f) - (point +  variance * point.y);
	l.w = 0.0f;
	l = normalize(l);
	n = vec4(normal, 0.0f);
	//n = vec4(0.0f, 0.0f, 1.0f, 0.0f);
	//n = rotationMatrix * n;
*/
}
