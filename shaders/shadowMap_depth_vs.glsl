#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 LSM;

void main() {
    gl_Position = LSM * model * vec4(position, 1.0);
}