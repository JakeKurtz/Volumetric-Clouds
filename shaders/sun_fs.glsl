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

uniform vec2 iResolution;

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
float remap(float x, float low1, float high1, float low2, float high2){
	return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}
vec3 luminance(vec3 col) {
    float R = col.r;
    float G = col.g;
    float B = col.b;
    return vec3(0.299*R + 0.587*G + 0.114*B);
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
    float h = remap(pos.y, planetRadius, atmosphereRadius, 0.f, 1.f);
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

    float density_ray = atmoDensity(currentPos, scaleHeight_rayleigh);
    float density_mie = atmoDensity(currentPos, scaleHeight_mie);

    vec3 outScatter = exp(-lightOpticalDepth_mie*beta_mie)*exp(-lightOpticalDepth_ray*beta_ray)*exp(-lightOpticalDepth_ray*beta_ozone);

    vec3 inScatter_ray = density_ray * outScatter;
    vec3 inScatter_mie = density_mie * outScatter;

    vec3 color = ( outScatter ) * ( lightColor );

    return color;
}

void main() {

    vec3 rayPos = camPos;
    vec3 lightColor = directLightColor(rayPos, normalize(FragPos-camPos))*lightIntensity;

    FragColor = vec4(lightColor, 1.0);
} 