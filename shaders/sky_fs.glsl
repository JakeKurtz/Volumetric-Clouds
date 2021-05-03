#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 Texcoord;

// Camera //

uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camUp;
uniform vec3 camRight;
uniform float exposure;

uniform vec2 iResolution;

// Atmosphere Properties //

uniform vec3 skyTopColor;
uniform vec3 skyBottomColor;

const int MAX_VIEW_SAMPLES = 64;
const int MAX_LIGHT_SAMPLES = 20;

uniform float scale_height;
uniform float planetRadius;
uniform float scatteringIntensity;

uniform float scaleHeight_rayleigh;
uniform float scaleHeight_mie;

uniform float ray_intensity;
uniform float mie_intensity;
uniform float absorption_intensity;

uniform vec3 planetCenter;
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
uniform sampler2D groundTex;
uniform sampler2D celestialTex;
uniform sampler2D cameraDepthTexture;

// Animation //

uniform float time;

const int MAX_LIGHT_STEPS = 10;
const int MAX_CLOUD_STEPS = 50;
const float EPSILON = 0.001;

vec3 BETA_RAY = vec3(5.19673e-6, 12.1427e-6, 29.6453e-6)*ray_intensity;
vec3 BETA_MIE = vec3(21e-6)*mie_intensity;
vec3 BETA_OZONE = vec3(2.04e-5, 4.97e-5, 1.95e-6)*absorption_intensity;

float EXTINCTION_COEFF = 0.5;
const float SCATTERING_COEFF = 0.5;
const float ABSORPTION_COEFF = 0.5;

float bn;
float worldDepthLinear;
float bar;

// translate cloud shell
uniform mat4 t;

#define PI 3.14159265358979323846264338327
#define FRONT 0.f
#define INSIDE 1.f
#define BEHIND 2.f
#define MISS 3.f

// ------------------------------------------------------------ //
//                      General Methods                         //
// ------------------------------------------------------------ //

float rand(vec2 co) {
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

vec3 luminance(vec3 col) {
    float R = col.r;
    float G = col.g;
    float B = col.b;
    return vec3(0.299*R + 0.587*G + 0.114*B);
}

// ------------------------------------------------------------ //
//                      Phase Functions                         //
// ------------------------------------------------------------ //

float beer(float density) { return exp( -density * EXTINCTION_COEFF ); }

float HG_Phase(float g, vec3 lightDir, vec3 viewDir) {
    float theta = dot(lightDir, viewDir);
    float g2 = g*g;
    return 0.25*PI * ((1.f - g2)/pow((1.f + g2-(2*g*theta)),1.5f));
} 

float Rayleigh_Phase(vec3 lightDir, vec3 viewDir) {
	float theta = dot(lightDir, viewDir);
	return 0.1875*PI * (1.0 + (theta*theta));
}

// ------------------------------------------------------------ //
//                  Sphere Intersection Methods                 //
// ------------------------------------------------------------ //

bool solveQuadratic(float a, float b, float c, float d, out float t0, out float t1) {
    if (d > 0.0) {
		t0 = max((-b - sqrt(d))/(2.0*a), 0.0);
		t1 = (-b + sqrt(d))/(2.0*a);
        return true;
	} else {
		t0 = 1e32;
		t1 = 0;
		return false;
	}
}

bool sphereIntersect(vec3 center, float radius, vec3 rayOrigin, vec3 rayDir, out float t0, out float t1) {
	vec3 L = rayOrigin - center;
	
	float a = dot(rayDir, rayDir);
	float b = 2.0 * dot(rayDir, L);
	float c = dot(L, L) - (radius * radius);
	float d = (b*b) - 4.0*a*c;

    return solveQuadratic(a,b,c,d,t0,t1);
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

float getIntersectionPosition(vec3 rayPos, vec3 rayDir, vec3 center, float radius, out float t0, out float t1) {
    if (sphereIntersect(center, radius, rayPos, rayDir, t0, t1)) {
        if (t1 > 0.f) {
            if (t0 > 0.f) {
                return FRONT;
            } else {
                return INSIDE;
            }        
        } else {
            return BEHIND;        
        }
    } else if (t1 == t0) {
        return BEHIND;
    } else {
        return MISS;
    }
}

vec2 cloudShellIntersection(vec3 rayPos, vec3 rayDir) {

    float t0, t1;
    float hit_outerShell = getIntersectionPosition(rayPos, rayDir, cloudCenter, cloudRadius+thickness, t0, t1);

    if (hit_outerShell != MISS || hit_outerShell != BEHIND) {
        if (hit_outerShell == INSIDE) {
            return vec2(0.f, t1);
        } else if (hit_outerShell == FRONT) {
            return vec2(t0, t1-t0);
        }
    } else {
        return vec2(0.f, 0.f);
    }
}

// ------------------------------------------------------------ //
//              Volumetric Atmosphere Methods                   //
// ------------------------------------------------------------ //

float atmoDensity(vec3 pos, float scaleHeight) {
    float h = max(0.f, length(pos - planetCenter) - planetRadius);
	return exp(-(h / scaleHeight));
}

void atmoRayLight(vec3 rayOrigin, vec3 rayDir, float rayLength, out float lightOpticalDepth_ray, out float lightOpticalDepth_mie) {
	float marchPos = 0.0;
	float stepSize = rayLength / float(MAX_LIGHT_SAMPLES);

    rayOrigin += (bn*stepSize*0.01);

	lightOpticalDepth_ray = 0.f;
	lightOpticalDepth_mie = 0.f;

	for (int i = 0; i < MAX_LIGHT_SAMPLES; i++) {
		vec3 densitySamplePoint = rayOrigin + rayDir * (marchPos);

        float density_ray = atmoDensity(densitySamplePoint, scaleHeight_rayleigh) * stepSize;
        float density_mie = atmoDensity(densitySamplePoint, scaleHeight_mie)* stepSize;

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

		return exp( -(BETA_RAY*(lightOpticalDepth_ray + viewOpticalDepth_ray) +
                      BETA_MIE*(lightOpticalDepth_mie + viewOpticalDepth_mie) + 
                      BETA_OZONE*(lightOpticalDepth_ray + viewOpticalDepth_ray)) );
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

		vec3 currentPos = rayOrigin + rayDir * (marchPos);

		float density_ray = atmoDensity(currentPos, scaleHeight_rayleigh) * stepSize;
		float density_mie = atmoDensity(currentPos, scaleHeight_mie) * stepSize;

        vec3 transmittance = getTransmittance(currentPos, lightDir, viewOpticalDepth_ray, viewOpticalDepth_mie);

		viewOpticalDepth_ray += density_ray;
		viewOpticalDepth_mie += density_mie;

        inScatter_ray += density_ray * transmittance;
        inScatter_mie += density_mie * transmittance;

		marchPos += stepSize;
	}

    opacity = exp(-(BETA_MIE * viewOpticalDepth_mie + BETA_RAY * viewOpticalDepth_ray + BETA_OZONE * viewOpticalDepth_ray));

	return ((inScatter_ray * BETA_RAY * phase_ray) + (inScatter_mie * BETA_MIE * phase_mie)) * (lightIntensity * lightColor) + vec3(0.f) * opacity;
}

// ------------------------------------------------------------ //
// SDF Primitive Combinations
// source: https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
// ------------------------------------------------------------ //

float sdSphere( vec3 p, float s ) {
  return length(p)-s;
}

float opUnion( float d1, float d2 ) { return min(d1,d2); }

float opSubtraction( float d1, float d2 ) { return max(-d1,d2); }

float opIntersection( float d1, float d2 ) { return max(d1,d2); }

float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }

float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

float opSmoothIntersection( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h); }

// ------------------------------------------------------------ //
//                  Volumetric Cloud Methods                    //
// ------------------------------------------------------------ //

vec3 directLightColor(vec3 currentPos, vec3 lightDir) {
	float s0, lightRayLength;
	sphereIntersect(planetCenter, atmosphereRadius, currentPos, lightDir, s0, lightRayLength);

    if (lightRayLength > 0) {
        lightRayLength -= s0;
    }

    float lightOpticalDepth_ray, lightOpticalDepth_mie; 
    atmoRayLight(currentPos, lightDir, lightRayLength, lightOpticalDepth_ray, lightOpticalDepth_mie);

    float density_ray = atmoDensity(currentPos, scaleHeight_rayleigh);
    float density_mie = atmoDensity(currentPos, scaleHeight_mie);

    vec3 outScatter = exp(-lightOpticalDepth_mie*BETA_MIE)*exp(-lightOpticalDepth_ray*BETA_RAY)*exp(-lightOpticalDepth_ray*BETA_OZONE);

    vec3 inScatter_ray = density_ray * outScatter;
    vec3 inScatter_mie = density_mie * outScatter;

    vec3 color = ( outScatter ) * ( lightColor );

    return outScatter*lightColor;
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

bool cloudDensity(vec3 p, out float density, out float dist) {
    p = vec3(t*vec4(p,1.0));

    float minHeight = (normalize(p)*cloudRadius).y;
    float maxHeight = (normalize(p)*(cloudRadius + thickness)).y;

    float p_h = remap(p.y, minHeight, maxHeight, 0.f, 1.f);

    float SR_b = saturate(remap(p_h, 0.0, cloudBottomRoundness, 0.f, 1.f));
    float SR_t = saturate(remap(p_h, 1.0*cloudTopRoundness, 1.0, 1.f, 0.f));
        
    float DR_b = p_h * saturate(remap(p_h, 0.0, bottomDensity, 0.f, 1.f));
    float DR_t = saturate(remap(p_h, topDensity, 1.f, 1.f, 0.f));

    float SA = SR_b * SR_t;
    float DA = DR_b * DR_t * 2;

    float innerShell = sdSphere( p, cloudRadius );
    float outerShell = sdSphere( p, cloudRadius + thickness );
    float cloudShell = opSubtraction( innerShell, outerShell ); 

    
    if (cloudShell < 0.f) {

        vec3 sphereNormal = normalize(p - cloudCenter);
        float u = atan(sphereNormal.x, sphereNormal.z) / (2 * PI) + 0.5;
        float v = asin(sphereNormal.y) / PI + 0.5;

        vec4 cns =  texture(coverageTex, vec2(u - (time * 0.05f),v)*coverageScale);
        vec4 sns =  texture(noisetex, p*noiseScale+(time*1.f));
        vec4 dns =  texture(detailNoiseTex, p*detailScale+(time*10.f));

        float DN = dns.r*0.625+dns.g*0.25+dns.b*0.125;
        float SN = sns.g*0.625+sns.b*0.25+sns.a*0.125;

        float coverageNoise = remap(cns.x*coverageIntensity, 0.f, 1.f, -1.f, 1.f);
        float shapeNoise = remap(sns.r, SN-1.f, 1, 0.f, 1.f)*noiseIntensity;

        shapeNoise = saturate(remap(shapeNoise*SA, 1-cns.x, 1, 0, 1));

        float detailNoise = 0.35*exp(-cns.x*0.75)*mix(DN, 1.f-DN, saturate(p_h*3.f))*detailIntensity;

        float noise = saturate(remap(shapeNoise, detailNoise, 1.f, 0.f, 1.f));

        cloudShell = opSubtraction( coverageNoise, cloudShell );
        cloudShell = noise * cloudShell;

        if (cloudShell < 0.f) {
            float sdfMultiplier = min(abs(cloudShell), 1.f)*DA;
            density = sdfMultiplier;
            dist = cloudShell;
            return true;
        } else {
            density = 0.f;
            dist = cloudShell;
            return false;
        }
    } else {
        density = 0.f;
        return false;
    } 
}

vec3 cloudRayLight(vec3 rayDir, vec3 rayPos) {

    float stepSizeOrig = thickness/float(MAX_LIGHT_STEPS);
    float stepSize = stepSizeOrig;

    float startRayOffset = bn*10.f*stepSize;
    vec3 currentPos = rayPos + rayDir;

    vec3 lightColor = directLightColor(currentPos, lightDir);

    float totalDist = 0.f;
    float totalDensity = 0.f;
    bool switchedBefore = false;
    for (int s = 0; s < MAX_LIGHT_STEPS; s++) {

        float ds;
        float dist;
        bool inCloud = cloudDensity(currentPos, ds, dist);
        if (inCloud) {
            totalDensity += ds * stepSize * density;
        }
        totalDist += inCloud ? stepSize : max(dist, stepSize);
        currentPos = rayPos + ((totalDist+startRayOffset)*lightDir);
    }

    vec3 light = vec3(0.f);

    for (int i = 0; i < 4; i++){

        float power = pow(0.5f,i);
        float phase_1 = HG_Phase(g*power, lightDir, rayDir);
        float phase_2 = HG_Phase((0.99-silver_spread)*power, lightDir, rayDir) * silver_intensity;

        float powder = 1.f - exp(-totalDensity*power*4.f);
        float beer = exp(-totalDensity*power) + 0.003*bn;

        light += power * SCATTERING_COEFF * beer * powder * lightColor * lightIntensity * max(phase_1, phase_2);
    }

    return light + ambientColor;
}

vec3 cloudRayMarch(vec3 rayDir, vec3 rayPos, out float depth, out float opacity) {

    depth = 0.f;
    opacity = 0.f;

    float transmittance = 1.f;
    vec3 scattering  = vec3(0.f);
    float totalDist = 0.f;
    float depthCount = 0.f;

    vec2 sphereInfo = cloudShellIntersection(rayPos, rayDir);

    float start = sphereInfo.x;
    float rayLength = min(sphereInfo.y,250);

    float stepSizeOrig = min(rayLength / float(MAX_CLOUD_STEPS), 1.f);

    //stepSizeOrig += (rayLength) * 0.001;
    float stepSize = stepSizeOrig;

    float startRayOffset = bn*20.f*stepSize;

    vec3 currentPos;
    bool exit = false;
    //while(totalDist < rayLength && totalDist+start < worldDepthLinear){
    while(totalDist < rayLength){

        stepSize = stepSizeOrig;
        currentPos = rayPos + (totalDist+start-0.001+startRayOffset)*rayDir;

        if ( totalDist+stepSize > rayLength || transmittance < 1e-4) {
            stepSize = rayLength - totalDist;
            exit = true;
        }

        float ds = 1.f;
        float dist;
        bool inCloud = cloudDensity(currentPos, ds, dist);

        stepSize *= 1.f / pow(max(ds, 0.4), 0.8); // increase step size based on density
        totalDist += inCloud ? stepSize : max(dist, stepSize);

        if (inCloud) {
            ds *= density;

            float transmittance_i = beer(ds * stepSize);
            float powder = 1 - beer(ds * stepSize * 4.f);

            opacity += (1.0 - transmittance_i) * (1.0 - opacity);

            transmittance *= transmittance_i;
            scattering += (transmittance * ds * powder * cloudRayLight(rayDir, currentPos));

            depth += (start+totalDist) * transmittance;
            depthCount += transmittance;
        }
        if (exit) break;
    }
    if (depthCount != 0.f) depth /= depthCount;
    depth = depth*(1.f-transmittance) + bar*(transmittance);
    return scattering;
}

void main() {

	vec2 uv = (gl_FragCoord.xy/iResolution.xy) - vec2(0.5);
    uv.x *= iResolution.x/iResolution.y;

    vec3 rayDir = mat3(camRight,camUp,camDir) * normalize(vec3(uv, 1.0));
    vec3 rayPos = camPos;

    bn = texture(blueNoise, uv*64.f).r;
    bn = fract(bn * 0.61803398875f);

    float worldDepth = texture(cameraDepthTexture, gl_FragCoord.xy/iResolution.xy).r;
    worldDepthLinear = linearize_depth(worldDepth, 0.1f, 1000000.f);

    float dstInsideAtmo, dstToAtmo;
    atmoIntersection(rayPos, rayDir, dstInsideAtmo, dstToAtmo);

    // Render clouds
    bar = min(worldDepthLinear, dstInsideAtmo);
    float cloudDepth;
    float cloudOpacity;
    vec3 cloudColor = cloudRayMarch(rayDir, rayPos, cloudDepth, cloudOpacity);

    // Render atmosphere
    float atmoRayLength = min(min(cloudDepth*2.5f, worldDepthLinear), dstInsideAtmo);
    vec3 atmoColor = vec3(0.f);

    vec3 atmoOpacity = vec3(1.f);
	if (atmoRayLength > 0) {
		vec3 pointInAtmo = rayPos + rayDir * dstToAtmo;
		atmoColor = 1.0 - exp(-atmoRayMarch(pointInAtmo, rayDir, atmoRayLength, atmoOpacity));
    }

    // Compute final image
    vec3 star = texture(celestialTex, gl_FragCoord.xy/iResolution.xy).rgb;
    atmoColor += worldDepthLinear > 50000 ? star*(1.f - cloudOpacity) : vec3(0.f);

    vec3 mainColor = texture(groundTex, gl_FragCoord.xy/iResolution.xy).rgb;

    // Blend in the atmosphere
    float ap_world = exp(-worldDepthLinear/ap_world_intensity);
    float ap_clouds = exp(-cloudDepth/ap_cloud_intensity);

    float lum_clouds = luminance(cloudColor).x;

    vec3 finalColor = (atmoColor+(mainColor*exp(-worldDepthLinear * 0.0001))) * (1.f - cloudOpacity);
    finalColor += (atmoColor+(cloudColor*exp(-cloudDepth*0.0001))) * cloudOpacity;

    FragColor = vec4(finalColor,1.f);
}