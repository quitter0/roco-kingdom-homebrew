#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D sceneTex;
uniform float bloomThreshold;   // brightness threshold (0.5–1.5)
uniform float bloomIntensity;   // bloom strength (0.0–2.0)
uniform vec2  texelSize;        // 1.0 / textureSize

void main()
{
    vec3 color = texture(sceneTex, fragTexCoord).rgb;

    // Luminance
    float lum = dot(color, vec3(0.2126, 0.7152, 0.0722));

    // Extract bright areas (soft threshold)
    float brightWeight = smoothstep(bloomThreshold - 0.3, bloomThreshold + 0.3, lum);

    // 9x9 box blur on the bright-pass
    vec3 bloom = vec3(0.0);
    float totalWeight = 0.0;
    for (int x = -4; x <= 4; x++)
    {
        for (int y = -4; y <= 4; y++)
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize * 2.5;
            vec3 sampleColor = texture(sceneTex, fragTexCoord + offset).rgb;
            float sampleLum = dot(sampleColor, vec3(0.2126, 0.7152, 0.0722));
            float weight = smoothstep(bloomThreshold - 0.3, bloomThreshold + 0.3, sampleLum);
            bloom += sampleColor * weight;
            totalWeight += weight;
        }
    }
    bloom = (totalWeight > 0.001) ? bloom / totalWeight : vec3(0.0);

    // Composite: original + bloom
    vec3 result = color + bloom * bloomIntensity * brightWeight;

    // Reinhard tone mapping (HDR → LDR)
    result = result / (result + vec3(1.0));

    // Gamma
    result = pow(result, vec3(1.0 / 2.2));

    finalColor = vec4(result, 1.0);
}
