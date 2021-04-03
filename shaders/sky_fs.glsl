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
const int MAX_LIGHT_SAMPLES = 4;

uniform float scale_height;
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

uniform float ap_world_intensity;
uniform float ap_cloud_intensity;

// Light Properties //
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform float lightIntensity;
uniform float ambientBlend;
uniform float g;
uniform float silver_intensity;
uniform float silver_spread;
uniform vec3 lightDir;

uniform float attinuationScalar;
uniform float attinuationClamp;

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

uniform float cloudRadius;
uniform vec3 cloudCenter;

uniform float max_steps;

// Textures //
uniform sampler3D noisetex;
uniform sampler3D detailNoiseTex;
uniform sampler2D coverageTex;
uniform sampler2D blueNoise;
uniform sampler2D cameraDepthTexture;
uniform sampler2D screenTexture;
uniform sampler2D starTex;

// Animation //
//float time = 0.0;
uniform float time;

const int MAX_LIGHT_STEPS = 6;
const int MAX_CLOUD_STEPS = 256;
float MAX_DIST = 2*(cloudRadius+thickness);
const float EPSILON = 0.001;

uniform float exposure;

#define PI 3.14159265358979323846264338327

// Scattering coefficients
vec3 beta_ray = vec3(5.19673e-6, 12.1427e-6, 29.6453e-6)*ray_intensity;
vec3 beta_mie = vec3(21e-6)*mie_intensity;
vec3 beta_ozone = vec3(2.04e-5, 4.97e-5, 1.95e-6)*absorption_intensity;

float bn;
float worldDepth;
float bar;
vec3 atmoColor = vec3(0.099,0.2,0.32);

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}
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
        return;
	} else {
		t0 = 1e32;
		t1 = 0;
		return;
	}
}
void sphereIntersect(vec3 center, float radius, vec3 rayOrigin, vec3 rayDir, out float t0, out float t1) {
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

    rayOrigin += (bn*stepSize*0.01);

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

        float t0, lightRayLength;
        sphereIntersect(planetCenter, atmosphereRadius, currentPos, lightDir, t0, lightRayLength);

        if (lightRayLength > 0) {
            lightRayLength -= t0;
        }

		float lightOpticalDepth_ray, lightOpticalDepth_mie;
		atmoRayLight(currentPos, lightDir, lightRayLength, lightOpticalDepth_ray, lightOpticalDepth_mie);

		return exp( -(beta_ray*(lightOpticalDepth_ray + viewOpticalDepth_ray) +
                      beta_mie*(lightOpticalDepth_mie + viewOpticalDepth_mie) + 
                      beta_ozone*(lightOpticalDepth_ray + viewOpticalDepth_ray)) );
}
vec3 atmoRayMarch(vec3 rayOrigin, vec3 rayDir, float rayLength, out vec3 opacity) {

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

    opacity = exp(-(beta_mie * viewOpticalDepth_mie + beta_ray * viewOpticalDepth_ray + beta_ozone * viewOpticalDepth_ray));

	return ((inScatter_ray * beta_ray * phase_ray) + (inScatter_mie * beta_mie * phase_mie)) * (lightIntensity * lightColor) + vec3(0.f) * opacity;
}

float cloudDensity(vec3 p, out float DA) {
    // translate cloud shell
    mat4 t = mat4(1.0, 0.0, 0.0, 0.0,  // 1. column
                  0.0, 1.0, 0.0, 0.0,  // 2. column
                  0.0, 0.0, 1.0, 0.0,  // 3. column
                  cloudCenter.x, cloudCenter.y, cloudCenter.z, 1.0); // 4. column

    p = vec3(inverse(t)*vec4(p,1.0));

    float minHeight = (normalize(p)*cloudRadius).y; // sphereCenter + dirTowardsIntersectionPoint * height
    float maxHeight = (normalize(p)*(cloudRadius + thickness)).y;

    float d1 = length(p)-(cloudRadius); // inner shell
    float d2 = length(p)-(cloudRadius + thickness); // outer shell 

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
    return (mainShape+noise+dmod);
}
float shellSDF(vec3 p) {
    // translate cloud shell
    mat4 t = mat4(1.0, 0.0, 0.0, 0.0,  // 1. column
                  0.0, 1.0, 0.0, 0.0,  // 2. column
                  0.0, 0.0, 1.0, 0.0,  // 3. column
                  cloudCenter.x, cloudCenter.y, cloudCenter.z, 1.0); // 4. column

    p = vec3(inverse(t)*vec4(p,1.0));

    float d1 = length(p)-cloudRadius; // inner shell
    float d2 = length(p)-(cloudRadius + thickness); // outer shell 

    return max(-d1, d2);
}

vec3 luminance(vec3 col) {
    float R = col.r;
    float G = col.g;
    float B = col.b;
    return vec3(0.299*R + 0.587*G + 0.114*B);
}
vec3 directLightColor(vec3 currentPos, vec3 lightDir) {

	float t0, lightRayLength;
    sphereIntersect(planetCenter, atmosphereRadius, currentPos, lightDir, t0, lightRayLength);

    if (lightRayLength > 0) {
        lightRayLength -= t0;
    }

    float lightOpticalDepth_ray; 
    float lightOpticalDepth_mie;
    atmoRayLight(currentPos, lightDir, lightRayLength, lightOpticalDepth_ray, lightOpticalDepth_mie);

    vec3 outScatter_ray = (beta_ray * lightOpticalDepth_ray) + (beta_ozone * lightOpticalDepth_ray) + (beta_mie * lightOpticalDepth_mie);

    return vec3(exp(-(outScatter_ray))*lightColor);
}
vec3 ambientLightColor(vec3 light) {
    return clamp(luminance(light), vec3(0.2), vec3(0.8));
    // TODO: make this better!
    //return vec3(0.5);
    //float R = atmoColor.r;
    //float G = atmoColor.g;
    //float B = atmoColor.b;
    //return vec3((R+R+B+B+G+G+G)/6);
    //return clamp(vec3(0.299*R + 0.587*G + 0.114*B), vec3(0.1), vec3(0.8));
}

void atmoIntersection(vec3 rayPos, vec3 rayDir, out float x, out float y) {

    float t0, t1;
	sphereIntersect(planetCenter, atmosphereRadius, rayPos, rayDir, t0, t1);
    if (t1 > 0) t1 -= t0;

    float s0, s1;
	sphereIntersect(planetCenter, planetRadius, rayPos, rayDir, s0, s1);
    if (s1 > 0) s1 -= s0;

	if (s1 < 0) {
		s0 = 1e32;
		s1 = 0;
	}

	x = min(t1, s0 - t0);
    y = t0;
}
void cloudShellIntersection(vec3 rayPos, vec3 rayDir, out float start, out float dist) {
    float t0, t1;
    sphereIntersect(cloudCenter, cloudRadius+thickness, rayPos, rayDir, t0, t1);

    float s0, s1;
    sphereIntersect(cloudCenter, cloudRadius, rayPos, rayDir, s0, s1);

    start = 0.f;
    dist = t1;

    if (t1 > 0 && t0 > 0) {
        dist = min(s0-t0, t1-t0);
        start = t0;
    }
    else if (t1 > 0 && t0 <= 0 && s0 <= 0) {
        dist = t1 - s1;
        start = s1;
    } else if (t1 > 0 && t0 <= 0 && s0 > 0) {
        dist = s0;
        start = 0.f;
    }
}

vec3 cloudRayLight(vec3 rayDir, vec3 rayPos, float stepSize) {
    vec3 currentPos = rayPos + (bn*stepSize);

    float totalDist = 0;

    float beer = 1.0;
    float powder = 1.0;

    vec3 light = vec3(0.0);

    vec3 lightColor = directLightColor(currentPos, lightDir);

	float phase_mie = max(HG_Phase(g, lightDir, rayDir), silver_intensity * HG_Phase(0.99-silver_spread, lightDir, rayDir));

    float totalDensity = 0.0;
    for (int s = 0; s < MAX_LIGHT_STEPS; s++) {

        float DA;
        float ds = cloudDensity(currentPos, DA);

        if (ds < 0) totalDensity += -ds * stepSize * density * DA;

        totalDist += stepSize;
        currentPos = rayPos + (totalDist*lightDir);

    }

    //beer = exp(-totalDensity*attinuationScalar);
    beer = max(exp(-totalDensity*attinuationScalar), exp(-attinuationClamp*attinuationScalar));
    beer = max(totalDensity*0.5, beer);
    powder = 1.0 - max(exp(-totalDensity*attinuationScalar*8), exp(-attinuationClamp*attinuationScalar*8));

    light = ambientColor + (beer * lightColor * lightIntensity * phase_mie);

    return light;
}
vec3 cloudRayMarch(vec3 rayDir, vec3 rayPos, out float depth, out float opacity) {

    depth = 0.f;
    opacity = 0.f;

    float beer = 1.f;
    float beer_i = beer;
    float depthCount = 0.f;

    float totalDist = 0.f;

    vec3 color = vec3(0.f);

    bool exit = false;

    float dstToCloudShell, dstInsideCloudShell;
    cloudShellIntersection(rayPos, rayDir, dstToCloudShell, dstInsideCloudShell);

    float rayLength = min(dstInsideCloudShell, worldDepth-dstToCloudShell);

    float stepSizeOrig = rayLength/float(MAX_CLOUD_STEPS) + dstToCloudShell * (0.0001);
    float stepSize = stepSizeOrig;

    vec3 rayStart = (rayPos + rayDir*dstToCloudShell);// + ((bn-0.5)*2*stepSize);
    vec3 currentPos = rayStart;

    float s0, s1;
	sphereIntersect(planetCenter, planetRadius, rayPos, rayDir, s0, s1);

    if (s1 == 0.f || s0 == 0.f) {
        for(int i = 0; i < MAX_CLOUD_STEPS; i++) {

            currentPos = rayStart + (totalDist*rayDir);

            if ( totalDist+stepSize > rayLength || beer <= 0.f) {
                stepSize = rayLength - totalDist;
                exit = true;
            }
            float DA;
            float densitySample = cloudDensity(currentPos, DA);

            if (densitySample < 0) {
                float ds = -densitySample * density * stepSize * DA;

                beer_i = exp(-ds*attinuationScalar);
                beer *= beer_i;

                depth += (dstToCloudShell+totalDist) * beer;
                depthCount += beer;

                opacity += (1.0 - beer_i) * (1.0 - opacity);
                vec3 light = cloudRayLight(rayDir, currentPos, 0.5);

                color += (ds * light * beer);
            }
            if (exit) break;
            stepSize = stepSizeOrig * 1.f/pow(max(0.4, -densitySample*stepSize*DA*density),0.8); // increase step size based on density
            totalDist += stepSize;
        }
    }
    if (depthCount != 0.f) depth /= depthCount;
    depth = depth*(1.f-beer) + bar*(beer);
    return color;
}

void main() {

	vec2 uv = (gl_FragCoord.xy/iResolution.xy) - vec2(0.5);
    uv.x *= iResolution.x/iResolution.y;

    vec3 rayDir = mat3(camRight,camUp,camDir) * normalize(vec3(uv, 1.0));
    vec3 rayPos = camPos;

    bn = texture(blueNoise, uv*50).r;
    worldDepth = texture(cameraDepthTexture, gl_FragCoord.xy/iResolution.xy).r;
    worldDepth = linearize_depth(worldDepth, 0.1f, 1000000.f);

    float dstInsideAtmo, dstToAtmo;
    atmoIntersection(rayPos, rayDir, dstInsideAtmo, dstToAtmo);
    //if (dstInsideAtmo > 0) {
	//	vec3 pointInAtmo = rayPos + rayDir * dstToAtmo;
	//	atmoColor = 1.0 - exp(-atmoRayMarch(pointInAtmo, rayDir, dstInsideAtmo));
    //}

    // Render clouds
    bar = min(worldDepth, dstInsideAtmo);
    float cloudDepth;//= 1e32f; 
    float cloudOpacity;// = 0.f;
    vec3 cloudColor = cloudRayMarch(rayDir, rayPos, cloudDepth, cloudOpacity);

    // Render atmosphere
    float atmoRayLength = min(min(cloudDepth, worldDepth), dstInsideAtmo);
    //vec3 atmoColor = vec3(0.f);

    vec3 atmoOpacity = vec3(1.f);
	if (atmoRayLength > 0) {
		vec3 pointInAtmo = rayPos + rayDir * dstToAtmo;
		atmoColor = 1.0 - exp(-atmoRayMarch(pointInAtmo, rayDir, atmoRayLength, atmoOpacity));
        //atmoColor *= lightIntensity;
    }

    //atmoColor = max(atmoColor, vec3(0.f,0.05f,0.1));

    // Compute final image
    //float test = atmoRayLength/dstInsideAtmo;
    //vec3 star = texture(screenTexture, gl_FragCoord.xy/iResolution.xy).rgb;
    //float x = saturate(luminance(atmoColor).r*2.5);
    //float y = saturate(luminance(star).r-x)*test;
    //atmoColor = star*y + atmoColor*x;

    vec3 mainColor = texture(screenTexture, gl_FragCoord.xy/iResolution.xy).rgb;

    // Blend in the atmosphere

    float ap_world = exp(-worldDepth/ap_world_intensity);
    float ap_clouds = exp(-cloudDepth/ap_cloud_intensity);

    //float test = mix(1.f, 0.f, pow(cloudDepth, ));

    vec3 finalColor = (atmoColor*(1.f-ap_world)+mainColor*ap_world) * (1.f - cloudOpacity);
    finalColor += (atmoColor*(1.f-ap_clouds)+cloudColor*ap_clouds) * cloudOpacity;

    FragColor = vec4(finalColor, 1.f);
}