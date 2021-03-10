#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float resolution_factor;
uniform float exposure;
uniform sampler2D screenTexture;
uniform sampler2D atmoColor;

void main()
{
    //vec4 col = texture(screenTexture, TexCoords*resolution_factor);
    //FragColor = col;

    const float gamma = 2.2;
    vec3 hdrColor = texture(screenTexture, TexCoords*resolution_factor).rgb;
  
    // reinhard tone mapping
    //vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // gamma correction 
    //vec3 mapped = pow(hdrColor, vec3(1.0 / gamma));

    // Narkowicz 2015, "ACES Filmic Tone Mapping Curve"
    vec3 v = hdrColor * 0.6f;
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    vec3 mapped = clamp((v*(a*v+b))/(v*(c*v+d)+e), 0.0f, 1.0f);

    FragColor = vec4(hdrColor, 1.0);
} 