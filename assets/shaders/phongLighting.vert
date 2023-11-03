#version 450

const uint NUM_LIGHTS = 1;

layout(binding = 0) uniform MatrixUBO
{
	mat4 model;
	mat4 viewProj;
	mat4 normal;
}
uMat;

layout(binding = 1) uniform SceneUBO
{
	vec3 cameraPos;
	vec3 lightPos[NUM_LIGHTS];
	vec3 lightColors;
}
uScene;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec2 inTangent;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outTexCoord;
layout(location = 2) out vec3 outFragPos;
layout(location = 3) out vec3 outViewPos;
layout(location = 4) out vec3 outLightPos;

void main()
{
	outFragPos = vec3(uMat.model * vec4(inPosition, 1.0));
	gl_Position = uMat.viewProj * vec4(outFragPos, 1.0);

	// we cannot simply multiply the normal vector by the model matrix,
	// because we shouldnt translate the normal vector
	// we use a normal matrix
	outNormal = mat3(uMat.normal) * inNormal;

	outTexCoord = inTexCoord;
	outViewPos = uScene.cameraPos;
	outLightPos = uScene.lightPos[0];
}