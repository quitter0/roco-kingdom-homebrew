#version 330

// Per-vertex attributes (standard raylib layout)
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexTangent;

// Per-instance attributes
in mat4 instanceTransform;
in vec4 instanceAnimParams;   // x=stateId, y/z/w=params
in vec4 instanceColor;

// Uniforms
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;
uniform float u_Time;

// Output to fragment shader
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec3 fragNormal;
out mat3 TBN;
out vec3 fragLocalPos;
out vec4 fragAnimParams;
out vec4 fragColor;

// ---- Procedural animation functions ----

vec3 animateIdle(vec3 pos, float time)
{
    float breathe = sin(time * 2.5) * 0.04;
    float swayX   = sin(time * 1.7 + 1.3) * 0.02;
    float swayZ   = cos(time * 1.9 + 0.7) * 0.02;
    pos.y += breathe;
    pos.x += swayX;
    pos.z += swayZ;
    return pos;
}

vec3 animateWalk(vec3 pos, float time, float speed)
{
    float cycle = time * speed * 6.0;
    float bounce = abs(sin(cycle)) * 0.08 * speed;
    pos.y += bounce;
    // forward lean proportional to speed
    pos.z += sin(cycle * 2.0) * 0.03 * speed;
    pos.x += cos(cycle * 1.3) * 0.02 * speed;
    return pos;
}

vec3 animateHit(vec3 pos, float hitTime, float intensity)
{
    float decay = exp(-hitTime * 5.0);
    float shakeX = sin(hitTime * 37.0) * decay * 0.06 * intensity;
    float shakeY = sin(hitTime * 43.0 + 1.2) * decay * 0.04 * intensity;
    float shakeZ = cos(hitTime * 41.0) * decay * 0.05 * intensity;
    pos += vec3(shakeX, shakeY, shakeZ);
    return pos;
}

vec3 animateSkill(vec3 pos, float progress, float power)
{
    // Charge phase: sink + slight rotation
    float charge = smoothstep(0.0, 0.5, progress);
    float burst  = smoothstep(0.5, 0.85, progress);
    float settle = smoothstep(0.85, 1.0, progress);

    // Sink down during charge
    float sink = charge * 0.08;
    pos.y -= sink;

    // Small Y-axis wobble during charge
    float wobbleAngle = sin(progress * 12.0) * charge * 0.15;
    float c = cos(wobbleAngle);
    float s = sin(wobbleAngle);
    pos.xz = mat2(c, -s, s, c) * pos.xz;

    // Burst forward + upward
    pos.z += burst * 0.3 * power;
    pos.y += burst * 0.12 * power;

    // Settle back
    pos.z -= settle * 0.3 * power;
    pos.y += settle * 0.08 * power;

    return pos;
}

vec3 applyNormalHit(vec3 n, float hitTime, float intensity)
{
    float decay = exp(-hitTime * 5.0);
    return normalize(n * (1.0 + decay * 0.25 * intensity));
}

vec3 applyNormalSkill(vec3 n, float progress, float power)
{
    float inflate = smoothstep(0.0, 0.5, progress) * 0.2 * power;
    return normalize(n * (1.0 + inflate));
}

void main()
{
    float stateId  = instanceAnimParams.x;
    float param1   = instanceAnimParams.y;
    float param2   = instanceAnimParams.z;

    vec3 pos = vertexPosition;
    vec3 nrm = vertexNormal;

    // Apply procedural animation based on state
    // State 0: Idle (always active as base)
    pos = animateIdle(pos, u_Time);

    if (stateId > 0.5 && stateId < 1.5) {
        // State 1: Walk (param1=speed)
        pos = animateWalk(pos, u_Time, param1);
    }

    if (stateId > 1.5 && stateId < 2.5) {
        // State 2: Hit (param1=hitTime, param2=intensity)
        pos = animateHit(pos, param1, param2);
        nrm = applyNormalHit(nrm, param1, param2);
    }

    if (stateId > 2.5 && stateId < 3.5) {
        // State 3: Skill (param1=progress, param2=power)
        pos = animateSkill(pos, param1, param2);
        nrm = applyNormalSkill(nrm, param1, param2);
    }

    // Apply instance transform (model matrix per-instance)
    vec4 worldPos = instanceTransform * vec4(pos, 1.0);

    // Tangent/binormal for TBN
    vec3 vertexBinormal = cross(vertexNormal, vertexTangent.xyz) * vertexTangent.w;
    mat3 normalMatrix3 = transpose(inverse(mat3(instanceTransform)));
    vec3 worldNormal = normalize(normalMatrix3 * nrm);

    fragPosition = worldPos.xyz;
    fragTexCoord = vertexTexCoord;
    fragNormal = worldNormal;

    vec3 fragTangent = normalize(normalMatrix3 * vertexTangent.xyz);
    fragTangent = normalize(fragTangent - dot(fragTangent, worldNormal) * worldNormal);
    vec3 fragBinormal = normalize(normalMatrix3 * vertexBinormal);
    fragBinormal = cross(worldNormal, fragTangent);

    TBN = transpose(mat3(fragTangent, fragBinormal, worldNormal));

    // Pass local position for SDF shadow
    fragLocalPos = pos;

    // Pass animation params to fragment shader (for shadow sync)
    fragAnimParams = instanceAnimParams;
    fragColor = instanceColor;

    gl_Position = mvp * worldPos;
}
