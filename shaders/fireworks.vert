#version 330 core
layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aColor;
layout (location = 2) in float aAlpha;
layout (location = 3) in float aSize;
uniform mat4 uView;
uniform mat4 uProjection;
out vec3 vColor;
out float vAlpha;
void main() {
    vec4 viewPosition = uView * vec4(aPosition, 1.0);
    gl_Position = uProjection * viewPosition;
    gl_PointSize = clamp(aSize * 42.0 / max(-viewPosition.z, 1.0), 1.0, 14.0);
    vColor = aColor;
    vAlpha = aAlpha;
}
