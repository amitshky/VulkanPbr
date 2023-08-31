#version 450 core

layout(location = 0) in vec3 aPos;

layout(binding = 0) uniform MatrixUBO
{
	mat4 model;
	mat4 viewProj;
	mat4 normal;
}
uMat;

layout(location = 0) out vec3 outTexCoords;

void main()
{
	outTexCoords = aPos;
	gl_Position = uMat.viewProj * vec4(aPos, 1.0);
}