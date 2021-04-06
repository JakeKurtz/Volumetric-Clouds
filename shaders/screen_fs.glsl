#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform float exposure;
//uniform float lum;
uniform sampler2D screenTexture;

vec3 reinhard(vec3 v)
{
    return v / (1.0f + v);
}

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 c_in, float l_out)
{
    float l_in = luminance(c_in);
    return c_in * (l_out / l_in);
}

vec3 reinhard_extended_luminance(vec3 v, float max_white_l)
{
    float l_old = luminance(v);
    float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0f + l_old);
    return change_luminance(v, l_new);
}

void main()
{
    vec2 TexelSize = 1.0 / textureSize(screenTexture, 0);
    vec3 hdrColor = texture(screenTexture, TexCoords).rgb;

    const float gamma = 2.2;
    //vec3 hdrColor = texture(hdrBuffer, TexCoords).rgb;
    //float _exposure = mix(exposure, 0.5 / lum, 0.05);
    // exposure tone mapping
    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // gamma correction 
    //mapped = pow(mapped, vec3(1.0 / gamma));
  
    FragColor = vec4(mapped, 1.0);
    //FragColor = vec4(reinhard_extended_luminance(hdrColor, exposure), 1.0);
} 