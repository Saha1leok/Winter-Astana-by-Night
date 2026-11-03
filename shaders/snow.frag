#version 330 core

in float vBrightness;
out vec4 fragmentColor;

void main() {
    vec2 center = gl_PointCoord - vec2(0.5);
    float distanceFromCenter = length(center);
    if (distanceFromCenter > 0.5) {
        discard;
    }
    float alpha = smoothstep(0.5, 0.18, distanceFromCenter) * 0.88;
    fragmentColor = vec4(vec3(0.78, 0.88, 1.0) * vBrightness, alpha);
}

