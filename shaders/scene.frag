#version 330 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;

uniform vec3 uBaseColor;
uniform vec3 uCameraPosition;
uniform bool uIsIce;
uniform bool uEmissive;
uniform bool uIsGround;
uniform float uEmissionStrength;
uniform float uSnowCoverage;

const int MAX_LAMPS = 12;
uniform int uLampCount;
uniform vec3 uLampPositions[MAX_LAMPS];
uniform float uLampStrengths[MAX_LAMPS];

out vec4 fragmentColor;

vec2 randomPoint(vec2 cell) {
    vec2 value = vec2(
        dot(cell, vec2(127.1, 311.7)),
        dot(cell, vec2(269.5, 183.3))
    );
    return fract(sin(value) * 43758.5453);
}

float cellularCrack(vec2 point) {
    vec2 cell = floor(point);
    vec2 local = fract(point);
    float nearest = 10.0;
    float secondNearest = 10.0;

    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 neighbour = vec2(float(x), float(y));
            vec2 feature = neighbour + randomPoint(cell + neighbour);
            float distanceToFeature = length(feature - local);
            if (distanceToFeature < nearest) {
                secondNearest = nearest;
                nearest = distanceToFeature;
            } else if (distanceToFeature < secondNearest) {
                secondNearest = distanceToFeature;
            }
        }
    }
    return 1.0 - smoothstep(0.006, 0.024, secondNearest - nearest);
}

float materialNoise(vec3 point) {
    vec3 cell = floor(point);
    cell = fract(cell * 0.1031);
    cell += dot(cell, cell.yzx + 33.33);
    return fract((cell.x + cell.y) * cell.z);
}

float softCastShadow(vec2 point, vec2 anchor, float shadowLength, float width) {
    // All shadows point away from the same moon/directional light.
    vec2 direction = normalize(vec2(0.35, -0.25));
    vec2 start = anchor + direction * 0.18;
    vec2 finish = anchor + direction * shadowLength;
    vec2 segment = finish - start;
    float t = clamp(dot(point - start, segment) / dot(segment, segment), 0.0, 1.0);
    float distanceToRay = length(point - (start + segment * t));
    float softEdge = 1.0 - smoothstep(width * 0.55, width, distanceToRay);
    return softEdge * (1.0 - 0.35 * t);
}

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
    vec3 lightDirection = normalize(vec3(-0.35, 1.0, 0.25));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 halfDirection = normalize(lightDirection + viewDirection);
    float specular = pow(max(dot(normal, halfDirection), 0.0), 48.0);
    bool snowSurface = uIsGround;
    vec3 shadingNormal = snowSurface ? vec3(0.0, 1.0, 0.0) : normal;
    diffuse = max(dot(shadingNormal, lightDirection), 0.0);
    halfDirection = normalize(lightDirection + viewDirection);
    specular = pow(max(dot(shadingNormal, halfDirection), 0.0), 48.0);
    // Snow receives strong blue night-sky fill. This keeps its relief visible
    // without producing hard black facets that look like incorrect shadows.
    float surfaceLight = snowSurface ? (0.76 + 0.24 * diffuse)
                                     : (0.31 + 0.72 * diffuse);
    vec3 color = uEmissive
        ? mix(vec3(0.025, 0.045, 0.075) * (0.34 + 0.66 * diffuse), uBaseColor * 1.35, uEmissionStrength)
        : uBaseColor * surfaceLight + vec3(0.42, 0.48, 0.62) * specular * 0.38;

    if (!uEmissive && !uIsIce) {
        float grain = materialNoise(vWorldPosition * 2.2);
        bool foliage = uBaseColor.g > uBaseColor.r * 1.7 && uBaseColor.g > uBaseColor.b * 1.25;
        if (foliage) {
            float needles = 0.86 + 0.14 * sin(vWorldPosition.y * 19.0 + vWorldPosition.x * 7.0);
            color *= needles * mix(0.94, 1.06, grain);
        } else if (abs(normal.y) < 0.45) {
            // Subtle facade/stone variation prevents perfectly flat plastic walls.
            float verticalBand = 0.96 + 0.04 * sin(vWorldPosition.y * 7.0);
            color *= verticalBand * mix(0.97, 1.03, grain);
        }

        // Universal slope-based snow: every upward-facing surface receives
        // coverage regardless of the object's original material or mesh.
        float upward = smoothstep(0.28, 0.74, normal.y);
        // A low-poly cone has steep geometric normals, while real fir branches
        // inside that cone present many small upward-facing needles to the snow.
        if (foliage) upward = max(upward, 0.78);
        float snowGrowth = smoothstep(0.0, 1.0, uSnowCoverage);
        float snowMask = clamp(upward * snowGrowth, 0.0, 1.0);
        // The terrain is already snow. Reapplying the animated accumulation
        // mask here creates long diagonal sine bands that resemble shadows.
        if (foliage || snowSurface) snowMask = 0.0;
        vec3 snowColor = vec3(0.90, 0.95, 1.04) * (0.78 + 0.22 * diffuse)
            + vec3(0.42, 0.48, 0.60) * specular * 0.40;
        color = mix(color, snowColor, snowMask * 0.98);
    }

    if (uIsIce) {
        float mainCracks = cellularCrack(vWorldPosition.xz * 0.48);
        float smallCracks = cellularCrack(vWorldPosition.zx * 0.92 + vec2(3.2, 1.4)) * 0.18;
        float shimmer = 0.5 + 0.5 * sin(vWorldPosition.z * 0.8 + vWorldPosition.x * 1.7);
        color += vec3(0.12, 0.17, 0.26) * max(mainCracks, smallCracks);
        color += vec3(0.025, 0.055, 0.095) * shimmer;
        float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0);
        color += vec3(0.08, 0.16, 0.22) * fresnel;
    }

    // Toggleable local Blinn-Phong lamps: diffuse and specular with attenuation.
    if (!uEmissive) {
        for (int i = 0; i < uLampCount; ++i) {
            vec3 toLight = uLampPositions[i] - vWorldPosition;
            float d = length(toLight);
            float attenuation = (1.0 - smoothstep(2.0, 4.2, d)) / (1.0 + 0.35*d + 0.40*d*d);
            vec3 localDirection = toLight / max(d, 0.001);
            float lambert = max(dot(shadingNormal, localDirection), 0.0);
            vec3 localHalf = normalize(localDirection + viewDirection);
            float localSpecular = pow(max(dot(shadingNormal, localHalf), 0.0), 56.0);
            vec3 warmLight = vec3(1.0, 0.62, 0.24);
            color += (uBaseColor * warmLight * 3.35 * lambert
                    + warmLight * 1.15 * localSpecular)
                   * attenuation * uLampStrengths[i];
        }
    }

    if (uIsGround) {
        float shadow = 0.0;
        // Landmark shadows.
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2(-9.35, 0.20), 4.4, 1.45));
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2( 0.20, 0.80), 5.2, 0.72));
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2(10.45, 0.40), 3.7, 1.70));
        // A few tree-group shadows provide contact and depth without striping the terrain.
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2(-12.3, 5.5), 1.7, 0.38));
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2( -6.4, 4.1), 1.8, 0.42));
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2(  8.6, 5.8), 1.6, 0.36));
        shadow = max(shadow, softCastShadow(vWorldPosition.xz, vec2( 12.4, 5.9), 1.6, 0.36));
        color *= 1.0 - shadow * 0.24;
    }

    float distanceFromCamera = length(uCameraPosition - vWorldPosition);
    float fogAmount = smoothstep(22.0, 48.0, distanceFromCamera);
    vec3 fogColor = vec3(0.025, 0.045, 0.095);
    color = mix(color, fogColor, fogAmount * 0.72);
    fragmentColor = vec4(color, 1.0);
}
