#version 330 core
layout (location = 0) in vec3 position;

uniform mat4 model;
uniform mat4 LSM;

out vec3 FragPos;

void main() {
    FragPos = vec3(model * vec4(position, 1.0));
    gl_Position = LSM * model * vec4(position, 1.0);
}