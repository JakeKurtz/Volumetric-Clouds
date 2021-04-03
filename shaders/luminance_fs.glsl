#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform bool last;
uniform sampler2D ToneMapSampler;

float luminance(vec3 v)
{
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

void main()
{
    
    const float EPSILON = 0.0001f;
    vec2 TexelSize = 1.0 / textureSize(ToneMapSampler, 0);
    
    if (!last) {

        float log_lum_sum = 0.f;

        log_lum_sum += log( luminance( texture(ToneMapSampler, TexCoords.xy + vec2(-1.f, -1.f)*TexelSize).rgb ) + EPSILON );
        log_lum_sum += log( luminance( texture(ToneMapSampler, TexCoords.xy + vec2(-1.f, 0.f)*TexelSize).rgb ) + EPSILON );
        log_lum_sum += log( luminance( texture(ToneMapSampler, TexCoords.xy + vec2(-1.f, 1.f)*TexelSize).rgb ) + EPSILON );

        //FragColor = vec4(vec3(log_lum_sum), 1.0);
        FragColor = texture(ToneMapSampler, TexCoords);

    } else {
        
        float sum = 0.f;

        sum += texture(ToneMapSampler, TexCoords.xy + vec2(1.5f, -1.5f)*TexelSize).r;
        sum += texture(ToneMapSampler, TexCoords.xy + vec2(1.5f, -0.5f)*TexelSize).r;
        sum += texture(ToneMapSampler, TexCoords.xy + vec2(1.5f, 0.5f)*TexelSize).r;

        sum = exp(sum / 16.f);

        //FragColor = vec4(vec3(sum), 1.0);
        FragColor = texture(ToneMapSampler, TexCoords);
    }
} 