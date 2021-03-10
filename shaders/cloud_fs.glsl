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
uniform float density;
uniform float topDensity;
uniform float bottomDensity;
uniform float coverageIntensity;
uniform float noiseIntensity;
uniform float detailIntensity;
uniform float coverageScale;
uniform float noiseScale;
uniform float detailScale;
uniform float thickness;
uniform float cloudTopRoundness;
uniform float cloudBottomRoundness;

uniform float cloudMinHeight;
uniform float cloudHeight;
uniform vec3 cloudCenter;

uniform int max_steps;

// Textures //
uniform sampler3D noisetex;
uniform sampler3D detailNoiseTex;
uniform sampler2D coverageTex;
uniform sampler2D cameraDepthTexture;
uniform sampler2D screenTexture;

// Animation //
uniform float time;

const int MAX_LIGHT_STEPS = 10;
const int MAX_CLOUD_STEPS = 64;
const float MAX_DIST = 300.0;
const float EPSILON = 0.001;

uniform float exposure;

#define PI 3.14159265358979323846264338327

// Scattering coefficients
vec3 beta_ray = vec3(5.5e-6, 13.0e-6, 22.4e-6)*ray_intensity;
vec3 beta_mie = vec3(21e-6)*mie_intensity;

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
		t0 = -1;//1e32;
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

float cloudDensity(vec3 p, out float DA) {
    // translate cloud shell
    mat4 t = mat4(1.0, 0.0, 0.0, 0.0,  // 1. column
                  0.0, 1.0, 0.0, 0.0,  // 2. column
                  0.0, 0.0, 1.0, 0.0,  // 3. column
                  cloudCenter.x, cloudCenter.y, cloudCenter.z, 1.0); // 4. column

    p = vec3(inverse(t)*vec4(p,1.0));

    float minHeight = (cloudCenter + normalize(p-cloudCenter)*cloudHeight).y; // sphereCenter + dirTowardsIntersectionPoint * height
    float maxHeight = (cloudCenter + normalize(p-cloudCenter)*(cloudHeight + thickness)).y;

    float d1 = distance(p, cloudCenter)-(cloudHeight); // inner shell
    float d2 = distance(p, cloudCenter)-(cloudHeight + thickness); // outer shell 

    vec4 sn = texture(noisetex, (p+time*5)*(noiseScale));
    vec4 dn = texture(detailNoiseTex, (p+time*5)*detailScale);

    float p_h = remap(p.y, minHeight, maxHeight, 0.f, 1.f);

    float SR_b = saturate(remap(p_h, 0.0, cloudBottomRoundness, 0.f, 1.f));
    float SR_t = saturate(remap(p_h, 1.0*cloudTopRoundness, 1.0, 1.f, 0.f));
        
    float DR_b = p_h * saturate(remap(p_h, 0.0, bottomDensity, 0.f, 1.f));
    float DR_t = saturate(remap(p_h, topDensity, 1.f, 1.f, 0.f));

    float SA = SR_b * SR_t;
    DA = DR_b * DR_t * 2;

    float cn = remap(texture(coverageTex, (p.xz+time*20)*coverageScale).x*coverageIntensity*SA, 0, 1, -1, 1);

    float DN = dn.r*0.625+dn.g*0.25+dn.b*0.125;

    float noise = remap(remap(sn.x,(sn.y*0.625+sn.z*0.25+sn.w*0.125)-1.f, 1, 0.f, 1.f)*noiseIntensity, 0, 1, -1, 1);
    float dmod = remap(0.35*mix(DN, 1.f-DN, saturate(p_h*5.f))*detailIntensity, 0, 1, -1, 1);

    float mainShape = remap(max(max(-d1,d2),-cn), 0, 1, -1, 1);
    return mainShape+noise-dmod;
}

vec3 directLightColor(vec3 currentPos, vec3 lightDir) {
	float s0, s1;
	atmoIntersect(planetCenter, atmosphereRadius, currentPos, lightDir, s0, s1);
	float lightRayLength = s1;

    float lightOpticalDepth_ray; 
    float lightOpticalDepth_mie;
    atmoRayLight(currentPos, lightDir, lightRayLength, lightOpticalDepth_ray, lightOpticalDepth_mie);

    vec3 outScatter_ray = beta_ray * lightOpticalDepth_ray;

    return vec3(exp(-(outScatter_ray)));
}
vec3 ambientLightColor() {
    return vec3(0.5);//clamp(vec3((atmoColor.x+atmoColor.y+atmoColor.z)/3.0), vec3(0.2), vec3(0.8));
}
vec3 cloudRayLight(vec3 rayDir, vec3 rayPos, float stepSize) {
    vec3 currentPos = rayPos;
    float totalDist = 0;
    float beer = 1.0;
    float powder = 1.0;
    vec3 light = vec3(0.0);

    vec3 lightColor = directLightColor(currentPos, lightDir);
    vec3 ambientLight = ambientLightColor();
	float phase_mie = mix(HG_Phase(g, lightDir, rayDir), HG_Phase(0.0, lightDir, rayDir), 0.7);

    float totalDensity = 0.0;
    for (int s = 0; s < MAX_LIGHT_STEPS; s++) {
        //float ds = distfunc(currentPos);
        float DA;
        float ds = cloudDensity(currentPos, DA);
        if (ds < EPSILON) {
            
            totalDensity += -ds * density * DA;
        }
        totalDist += ds < EPSILON ? stepSize : max(ds, stepSize);
        //totalDist += stepSize;
        currentPos = rayPos + (totalDist*lightDir);
    }

    beer = exp(-stepSize*totalDensity);
    powder = 1.0 - exp(-stepSize*totalDensity*8);

    light = ambientLight + beer * powder * 0.1 * lightColor * lightIntensity * phase_mie;

    return light;
}
vec3 cloudRayMarch(vec3 rayDir, vec3 rayPos, float stepSize, float dstLimit, out float opacity) {
    float beer = 1.f;
    float beer_i = beer;

    vec3 color = vec3(0.f);
    float opacitySkew = 0.0;

    float totalDist = 0.f;
    vec3 currentPos = rayPos;
    float densitySample = 0.0;

    bool hitSphereSurface = false;
    vec3 sphereSurface = vec3(0.0);
    bool cloudHit = false;
    float depth = 0.f;
    bool exit = false;
    for(int i = 0 ; i < MAX_CLOUD_STEPS; i++) {

        if (totalDist+stepSize > dstLimit || beer == 0.0) {
            stepSize = dstLimit - totalDist;
            exit = true;
        }

        float DA;
        float densitySample = cloudDensity(currentPos, DA);

        if (densitySample < EPSILON) {

            if (!cloudHit) {
                cloudHit = true;
                depth = totalDist;
            }

            float ds = -densitySample * density * DA;
            beer_i = exp(-ds*stepSize);
            beer *= beer_i;

            opacity += (1.0 - beer_i) * (1.0 - opacity);
            vec3 light = cloudRayLight(rayDir, currentPos, 0.1);

            color += (light * ds * stepSize * beer);
        }
        // march forward
        totalDist += densitySample < EPSILON ? stepSize : max(densitySample, stepSize);
        //totalDist += stepSize;
        currentPos = rayPos + (totalDist*rayDir);

        if (exit) break;

    }

    // Skewing the absorption color towards the atmospheric color
    //float cos_theta = abs(dot(rayDir, vec3(0,1,0)));
    //float t = 1.0 - pow(0.0001, cos_theta);
    //float smoothLerp = pow(cos_theta,3) * (6*pow(cos_theta,2)-15*cos_theta + 10);
    //float ambientSkew = mix(ambientBlend, 0.0, t); // will blend more towards the horizon
    //opacity = (opacity - ambientSkew); + opacitySkew;

    //color += atmoColor * beer;

    return color;
}

void main() {

	vec2 uv = (gl_FragCoord.xy/iResolution.xy) - vec2(0.5);
    uv.x *= iResolution.x/iResolution.y;

    vec3 rayDir = mat3(camRight,camUp,camDir) * normalize(vec3(uv, 1.0));
    vec3 rayPos = camPos;

    float t0 = 0; 
    float t1 = 0;
    //atmoIntersect(vec3(0.f,0.f,0.f), 10, rayPos, rayDir, t0, t1);
    //float dstToSphere = max(0, t0);
    //float dstInsideSphere = max(0, t1);
   
    float depth = texture(cameraDepthTexture, gl_FragCoord.xy/iResolution.xy).r;
    depth = linearize_depth(depth, 0.1f, 1000000.f);

    float opacity = 0.f;
    float dstLimit = min(depth, MAX_DIST);
    float stepSize = MAX_DIST/float(MAX_CLOUD_STEPS);

    vec3 cloudColor = vec3(0.0);
    
    cloudColor = cloudRayMarch(rayDir, rayPos, stepSize, dstLimit, opacity);
    //col = (cloudColor);// + (col*(1-opacity));

    //depth /= max_steps;

    FragColor = vec4(cloudColor, opacity);
}