#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D texture_diffuse;

void main()
{
    FragColor = texture(texture_diffuse, TexCoord)*0.001;
} 