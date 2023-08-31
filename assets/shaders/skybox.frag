#version 450 core

layout(location = 0) in vec3 inTexCoords;

layout(binding = 1) uniform samplerCube uSkybox;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = texture(uSkybox, inTexCoords);
}