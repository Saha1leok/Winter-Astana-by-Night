#version 330 core

in vec3 vWorldPosition;
in vec3 vWorldNormal;

uniform vec3 uBaseColor;
uniform vec3 uCameraPosition;
uniform bool uIsIce;
uniform bool uEmissive;
uniform bool uIsSnow;
uniform bool uShowFrost;

out vec4 fragmentColor;

vec2 randomPoint(vec2 cell) {
    vec2 value = vec2(
        dot(cell, vec2(127.1, 311.7)),
        dot(cell, vec2(269.5, 183.3))
    );
    return fract(sin(value) * 43758.5453);
}

float noise(vec2 p) {
    vec2 cell = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);
    return mix(mix(randomPoint(cell).x, randomPoint(cell + vec2(1, 0)).x, u.x),
               mix(randomPoint(cell + vec2(0, 1)).x, randomPoint(cell + vec2(1, 1)).x, u.x), u.y);
}

float frostNoise(vec2 p) {
    return noise(p) * 0.57 + noise(p * 2.07 + 11.3) * 0.28
         + noise(p * 4.13 + 23.7) * 0.15;
}

float cellularCrack(vec2 point, float width) {
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
    float edge = secondNearest - nearest;
    // Screen-space filtering keeps fractures readable at street level and
    // prevents subpixel lines from flickering in the aerial view.
    float aa = max(fwidth(edge), 0.001);
    return 1.0 - smoothstep(width, width + aa, edge);
}

void main() {
    vec3 normal = normalize(vWorldNormal);
    vec3 lightDirection = normalize(vec3(-0.35, 1.0, 0.25));
    float diffuse = max(dot(normal, lightDirection), 0.0);
    vec3 color = uEmissive ? uBaseColor * 1.35 : uBaseColor * (0.34 + 0.66 * diffuse);

    if (uIsIce) {
        vec2 p = vWorldPosition.xz;
        // World-space projection is a procedural texture: no UV stretching
        // around bends, external image dependencies, or animated water waves.
        vec2 warp = vec2(noise(p * 0.7), noise(p * 0.7 + 8.4)) - 0.5;
        float cracks = cellularCrack(p * 0.82 + warp * 0.38, 0.016);
        float branches = cellularCrack(p.yx * 2.15 + warp * 0.2 + 3.2, 0.009);
        float clouds = frostNoise(p * vec2(0.64, 0.40));
        // Keep the shoreline formula in sync with River.cpp.
        float center = 1.75 + 1.30 * sin((p.y + 1.5) * 0.16);
        float halfWidth = 2.45 + 0.052 * (p.y + 12.0);
        float shoreDistance = max(halfWidth - abs(p.x - center), 0.0);
        float edgeFrost = (1.0 - smoothstep(0.20, 1.25, shoreDistance))
                         * (0.55 + 0.45 * clouds);
        float frost = uShowFrost ? max(smoothstep(0.50, 0.73, clouds) * 0.82, edgeFrost) : 0.0;
        vec3 deepIce = mix(vec3(0.075, 0.19, 0.28), uBaseColor, clouds * 0.60);
        color = mix(deepIce, vec3(0.62, 0.76, 0.84), frost) * (0.46 + 0.54 * diffuse);
        // Pale fractures remain visible on clear ice and soften under frost.
        color = mix(color, vec3(0.035, 0.12, 0.18), cracks * 0.50 * (1.0 - frost));
        color += vec3(0.27, 0.43, 0.50) * cracks * (0.65 - frost * 0.35);
        color += vec3(0.12, 0.20, 0.24) * branches * (1.0 - frost) * 0.32;
        vec3 viewDirection = normalize(uCameraPosition - vWorldPosition);
        vec3 halfDirection = normalize(lightDirection + viewDirection);
        float specular = pow(max(dot(normal, halfDirection), 0.0), mix(110.0, 20.0, frost));
        float fresnel = 0.04 + 0.96 * pow(1.0 - max(dot(normal, viewDirection), 0.0), 5.0);
        color += vec3(0.50, 0.65, 0.76) * specular * mix(0.65, 0.10, frost);
        color = mix(color, vec3(0.32, 0.45, 0.59), fresnel * (1.0 - frost * 0.65));
    }

    if (uIsSnow) {
        float grain = noise(vWorldPosition.xz * 22.0);
        float wind = noise(vWorldPosition.xz * vec2(0.8, 3.7));
        color *= 0.94 + 0.05 * wind + 0.025 * grain;
    }

    float distanceFromCamera = length(uCameraPosition - vWorldPosition);
    float fogAmount = smoothstep(22.0, 48.0, distanceFromCamera);
    vec3 fogColor = vec3(0.025, 0.045, 0.095);
    color = mix(color, fogColor, fogAmount * 0.72);
    fragmentColor = vec4(color, 1.0);
}


