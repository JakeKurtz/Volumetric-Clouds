#version 330 core
out vec4 FragColor;

uniform vec3 camPos;
uniform vec3 camDir;
uniform vec3 camUp;
uniform vec3 camRight;

uniform vec2 iResolution;

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

// Textures //
uniform sampler3D noisetex;
uniform sampler3D detailNoiseTex;
uniform sampler2D coverageTex;
uniform sampler2D cameraDepthTexture;

// Animation //
uniform float time;

const int MAX_LIGHT_STEPS = 10;
const int MAX_CLOUD_STEPS = 1000;
const float MAX_DIST = 300.0;
const float EPSILON = 0.001;

#define PI 3.14159265358979323846264338327

float saturate(float x){
	return clamp(x, 0.0, 1.0);
}
float remap(float x, float low1, float high1, float low2, float high2){
	return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}
float linearize_depth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
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
vec2 cloudRayMarch(vec3 rayDir, vec3 rayPos, float stepSize, float dstLimit) {
    float beer = 1.f;
    float beer_i = beer;

    float totalDist = 0.f;
    vec3 currentPos = rayPos;
    float densitySample = 0.0;

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
        }
        // march forward
        totalDist += densitySample < EPSILON ? stepSize : max(densitySample, stepSize);
        //totalDist += stepSize;
        currentPos = rayPos + (totalDist*rayDir);

        if (exit) break;
    }
    if (cloudHit == false) depth = 1e32;
    return vec2(depth, beer);
}

void main() {
	vec2 uv = (gl_FragCoord.xy/iResolution.xy) - vec2(0.5);
    uv.x *= iResolution.x/iResolution.y;

    vec3 rayDir = mat3(camRight,camUp,camDir) * normalize(vec3(uv, 1.0));
    vec3 rayPos = camPos;
   
    float depth = texture(cameraDepthTexture, gl_FragCoord.xy/iResolution.xy).r;
    depth = linearize_depth(depth, 0.1f, 1000000.f);

    float dstLimit = min(depth, MAX_DIST);
    float stepSize = MAX_DIST/float(MAX_CLOUD_STEPS);

    vec2 cloudDepth = cloudRayMarch(rayDir, rayPos, stepSize, dstLimit);

    FragColor = vec4(cloudDepth, 0.0, 1.0);
}