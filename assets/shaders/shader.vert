#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoords;

layout(binding = 1) uniform DynamicUBO
{
	mat4 model;
}
dUbo;

layout(location = 0) out vec2 outTexCoords;

void main()
{
	vec4 fragPos = dUbo.model * vec4(inPosition, 1.0);
	gl_Position = fragPos;

	outTexCoords = inTexCoords;
}