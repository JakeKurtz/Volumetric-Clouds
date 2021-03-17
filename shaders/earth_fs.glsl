#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
in vec4 FragPosLS;

//uniform float resolution_factor;
uniform vec3 lightDir;
uniform float lightIntensity;
uniform vec3 lightColor;
uniform vec3 camPos; 

uniform vec3 planetCenter;
uniform float planetRadius;
uniform float atmosphereRadius;

uniform float scaleHeight_rayleigh;
uniform float scaleHeight_mie;

uniform float ray_intensity;
uniform float mie_intensity;
uniform float absorption_intensity;

uniform sampler2D texture_diffuse;
uniform sampler2D shadowMap;
uniform sampler2D blueNoise;

vec3 beta_ray = vec3(5.19673e-6, 12.1427e-6, 29.6453e-6)*ray_intensity;
vec3 beta_mie = vec3(21e-6)*mie_intensity;
vec3 beta_ozone = vec3(2.04e-5, 4.97e-5, 1.95e-6)*absorption_intensity;

const int MAX_LIGHT_SAMPLES = 5;
const int MAX_VIEW_SAMPLES = 100;

#define PI 3.14159265358979323846264338327

float random(vec4 seed4) {
    float dot_product = dot(seed4, vec4(12.9898,78.233,45.164,94.673));
    return fract(sin(dot_product) * 43758.5453);
}

float linstep(float low, float high, float v) {
    return clamp((v-low)/(high-low), 0.0, 1.0);
}

float varShadowMapSample(vec2 coords, float compare) {
    vec2 moments = texture(shadowMap, coords.xy).xy;
    float p = step(compare, moments.x);
    float variance = max(moments.y - moments.x * moments.x, 0.00002);

    float d = compare - moments.x;
    float pMax = linstep(0.2, 1.0, variance / ( variance + d*d));

    return min(max(p, pMax), 1.f);
}

float sampleShadowMap(sampler2D map, vec2 coord, float compare) {
    return step(texture(map, coord).r, compare);
}

float shadowMix(sampler2D map, vec2 coord, vec2 texelSize, float compare){
    vec2 pos = coord/texelSize + vec2(0.5);
    vec2 fracpart = fract(pos);
    vec2 startTexel = (pos - fracpart) * texelSize;

    float bl_texel = sampleShadowMap(map, startTexel, compare);
    float br_texel = sampleShadowMap(map, startTexel + vec2(texelSize.x, 0.0), compare);
    float tl_texel = sampleShadowMap(map, startTexel + vec2(0.0, texelSize.y), compare);
    float tr_texel = sampleShadowMap(map, startTexel + texelSize, compare);
    
    float mixA = mix(bl_texel, tl_texel, fracpart.y);
    float mixB = mix(br_texel, tr_texel, fracpart.y);

    return mix(mixA, mixB, fracpart.x);
}

float offset_lookup(sampler2D map, vec3 loc, vec2 offset, float bias) {
    vec2 texmapscale = 1.0 / textureSize(map, 0);
    float pcfDepth = texture(map, loc.xy + offset * texmapscale).r;
    return loc.z - bias > pcfDepth  ? 1.0 : 0.0;
} 

vec2 remap(vec2 x, vec2 low1, vec2 high1, vec2 low2, vec2 high2){
	return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

void solveQuadratic(float a, float b, float c, float d, out float t0, out float t1) {
    if (d > 0.0) {
		t0 = max((-b - sqrt(d))/(2.0*a), 0.0);
		t1 = (-b + sqrt(d))/(2.0*a);

		if (t1 >= 0) {
			t1 = t1-t0;
			return;
		}

	} else {
		t0 = 1e32;
		t1 = 0;
		return;
	}
}
vec2 sphereIntersect(vec3 center, float radius, vec3 rayOrigin, vec3 rayDir) {
	vec3 L = rayOrigin - center;
	
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, L);
	float c = dot(L, L) - (radius * radius);
	float d = (b*b) - 4.0*a*c;

    float distToSphere, distInSphere;
    solveQuadratic(a,b,c,d,distToSphere,distInSphere);

    return vec2(distToSphere, distInSphere);
}
float atmoDensity(vec3 pos, float scaleHeight) {
	float h = length(pos) - planetRadius;
	return exp(-(h / scaleHeight));
}
void atmoRayLight(vec3 rayOrigin, vec3 rayDir, float rayLength, out float lightOpticalDepth_ray, out float lightOpticalDepth_mie) {
	float marchPos = 0.0;
	float stepSize = rayLength / float(MAX_LIGHT_SAMPLES);

	lightOpticalDepth_ray = 0.f;
	lightOpticalDepth_mie = 0.f;

	for (int i = 0; i < MAX_LIGHT_SAMPLES; i++) {
		vec3 densitySamplePoint = rayOrigin + rayDir * (marchPos + 0.5 * stepSize);

		float density_ray = atmoDensity(densitySamplePoint, scaleHeight_rayleigh) * stepSize;
		float density_mie = atmoDensity(densitySamplePoint, scaleHeight_mie) * stepSize;

		lightOpticalDepth_ray += density_ray;
		lightOpticalDepth_mie += density_mie;

		marchPos += stepSize;
	}
}
vec3 directLightColor(vec3 currentPos, vec3 lightDir) {
	float s0, s1;
	float lightRayLength = sphereIntersect(planetCenter, atmosphereRadius, currentPos, lightDir).y;

    if (lightRayLength > 0) {
        lightRayLength -= s0;
    }

    float lightOpticalDepth_ray, lightOpticalDepth_mie; 
    atmoRayLight(currentPos, lightDir, lightRayLength, lightOpticalDepth_ray, lightOpticalDepth_mie);

    vec3 outScatter = exp(lightOpticalDepth_mie*beta_mie)*exp(-lightOpticalDepth_ray*beta_ray)*exp(-lightOpticalDepth_ray*beta_ozone);

    vec3 color = ( outScatter ) * ( lightColor );
    //color = clamp(color, vec3(0.f), vec3(1.f));
    //color = remap(color, vec3(0.f), vec3(lightIntensity), vec3(0.f), vec3(1.f));

    return color;
}

// https://learnopengl.com/code_viewer_gh.php?code=src/5.advanced_lighting/3.1.3.shadow_mapping/3.1.3.shadow_mapping.fs
float shadowCalculation(vec4 fragPosLightSpace) {
    // perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    // transform to [0,1] range
    // calculate bias (based on depth map resolution and slope)
    vec3 normal = normalize(Normal);
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r; 
    float currentDepth = projCoords.z;

    //float epsilon = remap(currentDepth, 1.f, 250.f, 0.f, 1.f) * ; 

    float bias = max(texelSize.x * (1.0 - dot(normal, lightDir)), texelSize.x*5);
    float cosTheta = clamp(dot(normal,lightDir), 0.f, 1.f);
    //float bias = clamp(0.005*tan(acos(cosTheta)), 0.f, 0.001);

    //vec2 bn = texture(blueNoise, gl_FragCoord.xy*0.02).xy*0.01;

    return varShadowMapSample(projCoords.xy, projCoords.z);
}

void main() {
    vec3 lightColor = directLightColor(FragPos, lightDir)*2.f;
    //vec3 objectColor = vec3(0.5,0.5,0.5);
    vec3 objectColor = texture(texture_diffuse, TexCoord).xyz;
    // ambient
    float ambientStrength = 1.0;
    //vec3 ambient = vec3(0.1,0.2,0.3);//1.0 * lightColor;
    vec3 ambient = vec3(0.1,0.2,0.33)*ambientStrength;//1.0 * lightColor;
  	
    // diffuse 
    vec3 norm = normalize(Normal);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(FragPos - camPos);
    vec3 reflectDir = reflect(lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;  

    float shadow = shadowCalculation(FragPosLS);
        
    vec3 result = (ambient + (shadow) * diffuse) * objectColor;

    FragColor = vec4(result, 1.0);
} 