#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 FragPos;
out vec3 Normal;
out vec2 Texcoord;
out mat3 TBN;

void main() {
    FragPos = vec3(model * vec4(position, 1.0));
    Normal = normal;
    Texcoord = texcoord;

    gl_Position = projection * view * model * vec4(position, 1.0);
}