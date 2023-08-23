#version 450

const uint NUM_LIGHTS = 4;

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
uMat;

layout(binding = 1) uniform SceneUBO
{
	vec3 cameraPos;
	vec3 lightPos[NUM_LIGHTS];
	vec3 lightColors;
}
uScene;

layout(location = 0) out VsOut
{
	vec2 texCoords;
	vec3 tangentFragPos;
	vec3 tangentCameraPos;
	vec3 tangentLightPos[NUM_LIGHTS];
	vec3 lightColors;
}
vsOut;

void main()
{
	vsOut.texCoords = aTexCoords;
	vec3 fragPos = vec3(uMat.model * vec4(aPosition, 1.0));

	vec3 T = normalize(vec3(uMat.model * vec4(aTangent, 0.0)));
	vec3 N = normalize(vec3(uMat.model * vec4(aNormal, 0.0)));
	// re-orthoganize T with respect to N
	// this is done because the fragment shader interpolation will smooth out the tangent vectors
	T = normalize(T - dot(T, N) * N);
	vec3 B = cross(N, T);
	// for orthogonal matrices transpose = inverse (transpose is faster than inverse)
	mat3 invTBN = transpose(mat3(T, B, N));
	// TBN mat converts tangent space into world space
	// we inverse the TBN matrix to convert world space to tangent space
	// to convert all the lighitng variables (except normals) into tangent space

	vsOut.tangentFragPos = invTBN * fragPos;
	vsOut.tangentCameraPos = invTBN * uScene.cameraPos;
	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		vsOut.tangentLightPos[i] = invTBN * uScene.lightPos[i];
	}
	vsOut.lightColors = uScene.lightColors;

	gl_Position = uMat.viewProj * vec4(fragPos, 1.0);
}