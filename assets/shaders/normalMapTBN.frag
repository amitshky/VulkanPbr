#version 450

const uint NUM_LIGHTS = 4;

layout(binding = 1) uniform SceneUBO
{
	vec3 cameraPos;
	vec3 lightPos[NUM_LIGHTS];
	vec3 lightColors;
}
scene;

layout(binding = 2) uniform sampler2D textureMaps[5];

layout(location = 0) in FsIn
{
	vec2 texCoords;
	vec3 fragPos;
	mat3 TBN;
}
fsIn;

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

vec3 Pbr(vec3 albedo, float roughness, float metallic, float ao, vec3 normal)
{
	vec3 viewDir = normalize(scene.cameraPos - fsIn.fragPos);

	vec3 Lo = vec3(0.0);
	// in metallic workflow, we assume that most dielectrics (non-metals) look visually similar with reflectivity
	// 0.04 and for metals, reflectivity is based on the albedo of the metal so we linearly interpolate between them
	// based on the value of `metallic` parameter
	vec3 reflectivity = vec3(0.04);
	reflectivity = mix(reflectivity, albedo, metallic);

	for (int i = 0; i < NUM_LIGHTS; ++i)
	{
		// calc per-light radiance
		vec3 lightVec = scene.lightPos[i] - fsIn.fragPos;
		vec3 lightDir = normalize(lightVec);
		vec3 halfway = normalize(viewDir + lightDir);
		float dist = length(lightVec);
		float attenuation = 1.0 / (dist * dist);
		vec3 radiance = scene.lightColors * attenuation;

		// Cook-Torrance BRDF
		float NDF = DistributionGGX(normal, halfway, roughness);
		float G = GeometrySmithGGX(normal, viewDir, lightDir, roughness);
		vec3 F = FresnelSchlick(clamp(dot(halfway, viewDir), 0.0, 1.0), reflectivity);

		vec3 numerator = NDF * G * F;
		// 0.0001 to prevent division by zero
		float denominator = 4.0 * max(dot(normal, viewDir), 0.0) * max(dot(normal, lightDir), 0.0) + 0.0001;
		vec3 specular = numerator / denominator;

		// energy conservation
		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic; // metals have no diffuse light

		// reflectance equation
		float nDotL = max(dot(normal, lightDir), 0.0);
		Lo += ((kD * albedo / PI) + specular) * radiance * nDotL;
	}

	vec3 ambient = vec3(0.01) * albedo * ao;
	vec3 color = ambient + Lo;

	return color;
}

void main()
{
	vec3 albedo = pow(texture(textureMaps[0], fsIn.texCoords).rgb, vec3(2.2));
	float roughness = texture(textureMaps[1], fsIn.texCoords).r;
	float metallic = texture(textureMaps[2], fsIn.texCoords).r;
	float ao = texture(textureMaps[3], fsIn.texCoords).r;
	vec3 normal = texture(textureMaps[4], fsIn.texCoords).rgb;
	normal = (normal * 2.0 - 1.0); // [0, 1] to [-1, 1]
	normal = normalize(fsIn.TBN * normal);

	vec3 color = Pbr(albedo, roughness, metallic, ao, normal);

	// tone mapping
	color = color / (color + vec3(1.0));
	// gamma correction
	color = pow(color, vec3(1.0 / 2.2));

	outColor = vec4(color, 1.0);
}