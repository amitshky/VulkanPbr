#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

layout(binding = 0) uniform DynamicUBO
{
	mat4 model;
	mat4 viewProj;
	mat4 normalMat;
}
dUbo;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoords;
layout(location = 2) out vec3 outFragPos;
layout(location = 3) out mat3 outTBN;

void main()
{
	outNormal = mat3(dUbo.normalMat) * aNormal;
	outTexCoords = aTexCoords;
	outFragPos = vec3(dUbo.model * vec4(aPosition, 1.0));

	vec3 T = normalize(vec3(dUbo.model * vec4(aTangent, 0.0)));
	vec3 B = normalize(vec3(dUbo.model * vec4(aBitangent, 0.0)));
	vec3 N = normalize(vec3(dUbo.model * vec4(aNormal, 0.0)));
	mat3 TBN = mat3(T, B, N);
	outTBN = TBN;

	gl_Position = dUbo.viewProj * vec4(outFragPos, 1.0);
}