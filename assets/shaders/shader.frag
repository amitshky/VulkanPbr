#version 450

layout(binding = 0) uniform UniformBufferObject
{
	vec3 cameraPos;
	vec3 albedo;
	float metallic;
	float roughness;
	float ao;
}
ubo;

layout(location = 0) out vec4 outColor;

void main()
{
	outColor = vec4(ubo.albedo, 1.0f);
}