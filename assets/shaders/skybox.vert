#version 450 core

layout(location = 0) in vec3 aPosition;
// unused
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;

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
	outTexCoords = aPosition;
	vec4 pos = uMat.viewProj * vec4(aPosition, 1.0);
	gl_Position = pos.xyww;
}