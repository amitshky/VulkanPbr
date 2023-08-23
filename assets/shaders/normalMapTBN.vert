#version 450

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;

layout(binding = 0) uniform MatrixUBO
{
	mat4 model;
	mat4 viewProj;
	mat4 normal;
}
mat;

layout(binding = 1) uniform SceneUBO
{
	vec3 cameraPos;
	vec3 lightPos[1];
	vec3 lightColors[1];
}
scene;

layout(location = 0) out VsOut
{
	vec2 texCoords;
	vec3 fragPos;
	mat3 TBN;
}
vsOut;

void main()
{
	vsOut.texCoords = aTexCoords;
	vsOut.fragPos = vec3(mat.model * vec4(aPosition, 1.0));

	vec3 T = normalize(vec3(mat.model * vec4(aTangent, 0.0)));
	vec3 N = normalize(vec3(mat.model * vec4(aNormal, 0.0)));
	// re-orthoganize T with respect to N
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	vsOut.TBN = mat3(T, B, N);

	gl_Position = mat.viewProj * vec4(vsOut.fragPos, 1.0);
}