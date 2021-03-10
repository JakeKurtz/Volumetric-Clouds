#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 Texcoord;

uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camUp;
uniform vec3 camRight;

uniform vec2 iResolution;

// Atmosphere Properties //
uniform vec3 skyTopColor;
uniform vec3 skyBottomColor;

const int MAX_VIEW_SAMPLES = 64;
const int MAX_LIGHT_SAMPLES = 10;

uniform float scale_height;
uniform float atmosphereHeight;
uniform float planetRadius;
uniform float scatteringIntensity;

uniform float scaleHeight_rayleigh;
uniform float scaleHeight_mie;

uniform float ray_intensity;
uniform float mie_intensity;
uniform float absorption_intensity;

uniform vec3 planetCenter;
//float atmosphereRadius = planetRadius * 1.025;
uniform float atmosphereRadius;

// Light Properties //
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform float lightIntensity;
uniform float ambientBlend;
uniform float g;
uniform vec3 lightDir;

// Cloud Properties //
uniform int max_steps;

uniform sampler2D cameraDepthTexture;
uniform sampler2D cloudDepthTexture;
uniform sampler2D screenTexture;

const int MAX_LIGHT_STEPS = 10;
const int MAX_CLOUD_STEPS = 2000;
const float MAX_DIST = 115.0;
const float EPSILON = 0.001;

uniform float exposure;

#define PI 3.14159265358979323846264338327

// Scattering coefficients
vec3 beta_ray = vec3(5.5e-6, 13.0e-6, 22.4e-6)*ray_intensity;
vec3 beta_mie = vec3(21e-6)*mie_intensity;
vec3 beta_ozone = vec3(2.04e-5, 4.97e-5, 1.95e-6)*absorption_intensity;

vec3 atmoColor = vec3(0);
vec3 atmoColor2 = vec3(0);

float saturate(float x){
	return clamp(x, 0.0, 1.0);
}
float remap(float x, float low1, float high1, float low2, float high2){
	return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}
float linearize_depth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

float HG_Phase(float g, vec3 lightDir, vec3 viewDir) {
    float theta = dot(lightDir, viewDir);
    float g2 = g*g;
    return 1.f/(4.f*PI) * ((1.f - g2)/pow((1.f + g2-(2*g*theta)),1.5f));
} 
float Rayleigh_Phase(vec3 lightDir, vec3 viewDir) {
	float theta = dot(lightDir, viewDir);
	return 3/(16*PI) * (1.0 + (theta*theta));
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
void atmoIntersect(vec3 center, float radius, vec3 rayOrigin, vec3 rayDir, out float t0, out float t1) {
	vec3 L = rayOrigin - center;
	
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, L);
	float c = dot(L, L) - (radius * radius);
	float d = (b*b) - 4.0*a*c;

    solveQuadratic(a,b,c,d,t0,t1);
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

vec3 getTransmittance(vec3 currentPos, vec3 lightDir, float viewOpticalDepth_ray, float viewOpticalDepth_mie) {

		float s0, s1;
		atmoIntersect(planetCenter, atmosphereRadius, currentPos, lightDir, s0, s1);
		float lightRayLength = s1;

		float lightOpticalDepth_ray, lightOpticalDepth_mie;
		atmoRayLight(currentPos, lightDir, lightRayLength, lightOpticalDepth_ray, lightOpticalDepth_mie);

		return exp( -(beta_ray*(lightOpticalDepth_ray + viewOpticalDepth_ray) +
                      beta_mie*(lightOpticalDepth_mie + viewOpticalDepth_mie) + 
                      beta_ozone*(lightOpticalDepth_ray + viewOpticalDepth_ray)) );
}
vec3 atmoRayMarch(vec3 rayOrigin, vec3 rayDir, float rayLength, bool mie) {

	float stepSize = rayLength / float(MAX_VIEW_SAMPLES);
	float marchPos = 0.0;

	float phase_ray = Rayleigh_Phase(lightDir, rayDir);
	float phase_mie = HG_Phase(0.99, lightDir, rayDir);

	float viewOpticalDepth_ray = 0.0;
	float viewOpticalDepth_mie = 0.0;

	vec3 inScatter_ray = vec3(0);
	vec3 inScatter_mie = vec3(0);

	for (int i = 0; i < MAX_VIEW_SAMPLES; ++i) {

		vec3 currentPos = rayOrigin + rayDir * (marchPos + 0.5 * stepSize);

		float density_ray = atmoDensity(currentPos, scaleHeight_rayleigh) * stepSize;
		float density_mie = atmoDensity(currentPos, scaleHeight_mie) * stepSize;

        vec3 transmittance = getTransmittance(currentPos, lightDir, viewOpticalDepth_ray, viewOpticalDepth_mie);

		viewOpticalDepth_ray += density_ray;
		viewOpticalDepth_mie += density_mie;

		inScatter_ray += density_ray * transmittance;
		inScatter_mie += density_mie * transmittance;

		marchPos += stepSize;
	}
	if (mie)
		return ((inScatter_ray * beta_ray * phase_ray) + (inScatter_mie * beta_mie * phase_mie)) * lightIntensity;
	else
		return (inScatter_ray * beta_ray * phase_ray) * lightIntensity;
}

void main() {

	vec2 uv = (gl_FragCoord.xy/iResolution.xy) - vec2(0.5);
    uv.x *= iResolution.x/iResolution.y;

    vec3 rayDir = mat3(camRight,camUp,camDir) * normalize(vec3(uv, 1.0));
    vec3 rayPos = camPos;
	float opacity = 0.f;

	float geometryDepth = texture(cameraDepthTexture, gl_FragCoord.xy/iResolution.xy).r;
    geometryDepth = linearize_depth(geometryDepth, 0.1f, 1000000.f);

	vec2 blah = texture(cloudDepthTexture, gl_FragCoord.xy/iResolution.xy).rg;
	float cloudDepth = blah.r;
	float cloudAttenuation = blah.g;

	float depth = min(cloudDepth, geometryDepth);

    vec3 mainImageCol = texture(screenTexture, gl_FragCoord.xy/iResolution.xy).rgb;

    float t0, t1;
	atmoIntersect(planetCenter, atmosphereRadius, rayPos, rayDir, t0, t1);

	float s0,s1;
	atmoIntersect(planetCenter, planetRadius, rayPos, rayDir, s0, s1);

	if (s1 < 0) {
		s0 = 1e32;
		s1 = 0;
	}

	t1 = min(t1, s0 - t0);
	float dstLimit = min(depth, t1);
	float totalLength =dstLimit;
	if (dstLimit == cloudDepth) {
		float bar = min(geometryDepth, t1);
		totalLength = ((bar - dstLimit) * cloudAttenuation) + dstLimit;
	}
	bool mie = true;
	if (totalLength > 0) {
		vec3 pointInAtmo = rayPos + rayDir * t0;
		if (totalLength == geometryDepth) mie = false;
		atmoColor = 1.0 - exp(-atmoRayMarch(pointInAtmo, rayDir, totalLength, mie));
    }

	vec3 finalColor = mainImageCol + atmoColor;

    FragColor = vec4(finalColor, 1.0);
}