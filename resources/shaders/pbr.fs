#version 330

#define PI          3.14159265358979323846
#define MAX_LIGHTS  5
#define LIGHT_DIR   0
#define LIGHT_POINT 1

// Input from vertex shader
in vec3 fragPosition;
in vec2 fragTexCoord;
in vec3 fragNormal;
in mat3 TBN;
in vec3 fragLocalPos;
in vec4 fragAnimParams;
in vec4 fragColor;

out vec4 finalColor;

// Material texture samplers
uniform sampler2D albedoMap;
uniform sampler2D metallicMap;
uniform sampler2D roughnessMap;
uniform sampler2D normalMap;
uniform sampler2D aoMap;

// Material parameters
uniform vec4  albedoColor;
uniform float metallicFactor;
uniform float roughnessFactor;

// Light structure
struct Light {
    int   enabled;
    int   type;
    vec3  position;
    vec3  color;
    float intensity;
};

uniform Light lights[MAX_LIGHTS];
uniform vec3  viewPos;
uniform vec3  ambientColor;

// Fresnel rim light
uniform float u_RimIntensity;
uniform vec3  u_RimColor;
uniform float u_RimPower;

// Pseudo-SDF ground shadow
uniform float u_ShadowY;
uniform float u_ShadowStrength;
uniform float u_ShadowSoftness;
uniform float u_Time;

// ---- BRDF functions ----

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a  = roughness * roughness;
    float a2 = a * a;
    float NdotH  = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    float denom  = NdotH2 * (a2 - 1.0) + 1.0;
    return a2 / max(PI * denom * denom, 0.00001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;
    return NdotV / max(NdotV * (1.0 - k) + k, 0.00001);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX(NdotV, roughness) *
           GeometrySchlickGGX(NdotL, roughness);
}

// ---- Pseudo-SDF ground shadow ----

float ComputeGroundShadow(vec3 worldPos, vec3 localPos)
{
    // Distance from this fragment to the ground plane
    float dist = worldPos.y - u_ShadowY;

    // Soft exponential falloff
    float softDist = dist / max(u_ShadowSoftness, 0.001);
    float shadow = exp(-softDist * softDist);

    // Shadow is darker near feet (low local Y)
    float footFalloff = smoothstep(0.5, -0.1, localPos.y);

    return shadow * footFalloff * u_ShadowStrength;
}

// ---- Main ----

void main()
{
    // Sample material maps
    vec4 albedoSample = texture(albedoMap, fragTexCoord);
    vec3 albedo = albedoSample.rgb * albedoColor.rgb * fragColor.rgb;

    float metallic  = texture(metallicMap,  fragTexCoord).b * metallicFactor;
    float roughness = texture(roughnessMap, fragTexCoord).g * roughnessFactor;
    float ao        = texture(aoMap,       fragTexCoord).r;

    metallic  = clamp(metallic,  0.04, 1.0);
    roughness = clamp(roughness, 0.04, 1.0);
    ao        = clamp(ao, 0.0, 1.0);

    // Normal
    vec3 N = normalize(fragNormal);
    vec4 normalSample = texture(normalMap, fragTexCoord);
    if (length(normalSample.rgb) > 0.01)
    {
        N = normalize(normalSample.rgb * 2.0 - 1.0);
        N = normalize(TBN * N);
    }

    vec3 V = normalize(viewPos - fragPosition);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Accumulate light from all enabled lights
    vec3 lightSum = vec3(0.0);

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 0) continue;

        vec3 L, radiance;
        if (lights[i].type == LIGHT_DIR)
        {
            L = normalize(lights[i].position);
            radiance = lights[i].color * lights[i].intensity;
        }
        else
        {
            vec3 toLight = lights[i].position - fragPosition;
            float dist = length(toLight);
            L = normalize(toLight);
            float attenuation = 1.0 / (1.0 + dist * dist * 0.08);
            radiance = lights[i].color * lights[i].intensity * attenuation;
        }

        vec3 H = normalize(V + L);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        float D = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);

        vec3 specular = (D * G * F) /
            max(4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0), 0.001);

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);
        float NdotL = max(dot(N, L), 0.0);

        lightSum += (kD * albedo / PI + specular) * radiance * NdotL;
    }

    // Ambient
    vec3 ambient = ambientColor * albedo * ao * 0.6;

    vec3 color = ambient + lightSum;

    // Fresnel rim light
    float NdotV = abs(dot(N, V));
    float fresnel = pow(1.0 - NdotV, u_RimPower);
    vec3 rim = u_RimColor * fresnel * u_RimIntensity;
    color += rim * albedo;

    // Pseudo-SDF ground soft shadow (syncs with vertex animation)
    float shadow = ComputeGroundShadow(fragPosition, fragLocalPos);
    color *= (1.0 - shadow * 0.55);

    // Gamma
    color = pow(color, vec3(1.0 / 2.2));

    finalColor = vec4(color, albedoSample.a * albedoColor.a * fragColor.a);
}
