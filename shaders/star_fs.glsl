#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

uniform sampler2D starTex;

void main()
{
    FragColor = texture(starTex, TexCoord);
} 