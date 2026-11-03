#version 330 core

layout (location = 0) in vec3 aPosition;

uniform mat4 uView;
uniform mat4 uProjection;

out float vBrightness;

void main() {
    vec4 viewPosition = uView * vec4(aPosition, 1.0);
    gl_Position = uProjection * viewPosition;
    gl_PointSize = clamp(105.0 / max(-viewPosition.z, 1.0), 2.0, 7.0);
    vBrightness = clamp(1.25 - (-viewPosition.z / 45.0), 0.45, 1.0);
}

