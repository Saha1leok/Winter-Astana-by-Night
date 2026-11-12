#version 330 core
in vec3 vColor;
in float vAlpha;
out vec4 fragmentColor;
void main() {
    float radius = length(gl_PointCoord - vec2(0.5));
    if (radius > 0.5) discard;
    float glow = 1.0 - smoothstep(0.05, 0.5, radius);
    fragmentColor = vec4(vColor * 1.6, vAlpha * glow);
}
