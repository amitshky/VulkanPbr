#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;

layout(binding = 1) uniform DynamicUBO
{
	mat4 model;
	mat4 viewProj;
	mat4 normalMat;
}
dUbo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoords;
layout(location = 2) out vec3 outFragPos;

void main()
{
	outNormal = mat3(dUbo.normalMat) * aNormal;
	outTexCoords = aTexCoords;
	outFragPos = vec3(dUbo.model * vec4(aPosition, 1.0));

	gl_Position = dUbo.viewProj * vec4(outFragPos, 1.0);
}