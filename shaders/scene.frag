#version 330 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;

uniform vec3 uBaseColor;
uniform vec3 uCameraPosition;
uniform bool uIsIce;
uniform bool uEmissive;

const int MAX_LAMPS = 12;
uniform int uLampCount;
uniform vec3 uLampPositions[MAX_LAMPS];

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
    return 1.0 - smoothstep(0.012, 0.040, secondNearest - nearest);
}

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDirection = normalize(vec3(-0.35, 1.0, 0.25));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 color = uEmissive ? uBaseColor * 1.35 : uBaseColor * (0.34 + 0.66 * diffuse);

    if (uIsIce) {
        float mainCracks = cellularCrack(vWorldPosition.xz * 0.48);
        float smallCracks = cellularCrack(vWorldPosition.zx * 0.92 + vec2(3.2, 1.4)) * 0.18;
        float shimmer = 0.5 + 0.5 * sin(vWorldPosition.z * 0.8 + vWorldPosition.x * 1.7);
        color += vec3(0.20, 0.28, 0.36) * max(mainCracks, smallCracks);
        color += vec3(0.018, 0.035, 0.060) * shimmer;
        vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
        float fresnel = pow(1.0 - max(dot(normal, viewDirection), 0.0), 3.0);
        color += vec3(0.08, 0.16, 0.22) * fresnel;
    }

    // Local diffuse light; no shadow maps or occlusion.
    if (!uEmissive) {
        for (int i = 0; i < uLampCount; ++i) {
            vec3 toLight = uLampPositions[i] - vWorldPosition;
            float d = length(toLight);
            float attenuation = (1.0 - smoothstep(2.0, 4.2, d)) / (1.0 + 0.35*d + 0.40*d*d);
            float lambert = max(dot(normal, toLight / max(d, 0.001)), 0.0);
            color += uBaseColor * vec3(1.0, 0.62, 0.24) * 2.8 * lambert * attenuation;
        }
    }

    float distanceFromCamera = length(uCameraPosition - vWorldPosition);
    float fogAmount = smoothstep(22.0, 48.0, distanceFromCamera);
    vec3 fogColor = vec3(0.025, 0.045, 0.095);
    color = mix(color, fogColor, fogAmount * 0.72);
    fragmentColor = vec4(color, 1.0);
}
