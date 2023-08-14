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

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inTexCoords;
layout(location = 2) in vec3 inFragPos;

layout(location = 0) out vec4 outColor;

const float PI = 3.14159265359;


vec3 FresnelSchlick(float cosTheta, vec3 baseReflectivity)
{
	return baseReflectivity + (1.0 - baseReflectivity) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 normal, vec3 halfway, float roughness)
{
	float a = roughness * roughness;
	float a2 = a * a;
	float nDotH = max(dot(normal, halfway), 0.0);
	float nDotH2 = nDotH * nDotH;
	float denominator = nDotH2 * (a2 - 1.0) + 1.0;
	denominator = PI * denominator * denominator;

	return a2 / denominator;
}

float GeometrySchlickGGX(vec3 normal, vec3 v, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;

	float nDotV = max(dot(normal, v), 0.0);

	return nDotV / (nDotV * (1.0 - k) + k);
}

float GeometrySmithGGX(vec3 normal, vec3 viewDir, vec3 lightDir, float roughness)
{
	float geometryView = GeometrySchlickGGX(normal, viewDir, roughness);
	float geometryLight = GeometrySchlickGGX(normal, lightDir, roughness);
	return geometryView * geometryLight;
}

void main()
{
	vec3 lightColors[4] = {
		vec3(10.0f, 10.0f, 10.0f),
		vec3(10.0f, 10.0f, 10.0f),
		vec3(10.0f, 10.0f, 10.0f),
		vec3(10.0f, 10.0f, 10.0f),
	};

	vec3 lightPos[4] = {
		vec3(0.0, 0.0, 3.0),
		vec3(0.0, 10.0, 0.0),
		vec3(10.0, 0.0, 0.0),
		vec3(-10.0, 0.0, 10.0),
	};

	vec3 normal = normalize(inNormal);
	vec3 viewDir = normalize(ubo.cameraPos - inFragPos);

	vec3 Lo = vec3(0.0);
	// in metallic workflow, we assume that most dielectrics (non-metals) look visually similar with reflectivity
	// 0.04 and for metals, reflectivity is based on the albedo of the metal so we linearly interpolate between them
	// based on the value of `metallic` parameter
	vec3 reflectivity = vec3(0.04);
	reflectivity = mix(reflectivity, ubo.albedo, ubo.metallic);

	for (int i = 0; i < 1; ++i)
	{
		// calc per-light radiance
		vec3 lightVec = lightPos[i] - inFragPos;
		vec3 lightDir = normalize(lightVec);
		vec3 halfway = normalize(viewDir + lightDir);
		float dist = length(lightVec);
		float attenuation = 1.0 / (dist * dist);
		vec3 radiance = lightColors[i] * attenuation;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(normal, halfway, ubo.roughness);
		float G = GeometrySmithGGX(normal, viewDir, lightDir, ubo.roughness);
		vec3 F = FresnelSchlick(clamp(dot(halfway, viewDir), 0.0, 1.0), reflectivity);

		vec3 numerator = NDF * G * F;
		// 0.0001 to prevent division by zero
		float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		// energy conservation
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - ubo.metallic; // metals have no diffuse light

		// reflectance equation
		float nDotL = max(dot(normal, lightDir), 0.0);
		Lo += ((kD * ubo.albedo / PI) + specular) * radiance * nDotL;
	}

	vec3 ambient = vec3(0.001) * ubo.albedo * ubo.ao;
	vec3 color = ambient + Lo;

	// tone mapping
	color = color / (color + vec3(1.0));
	// gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	outColor = vec4(color, 1.0);
}