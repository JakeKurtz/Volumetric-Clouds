#version 330 core

out vec4 FragColor;

// Camera //
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

uniform float cloudRadius;
uniform vec3 cloudCenter;

// Textures //
uniform sampler3D noisetex;
uniform sampler3D detailNoiseTex;
uniform sampler2D coverageTex;

// translate cloud shell
uniform mat4 t;
uniform float time;

const int MAX_CLOUD_STEPS = 5;
float EXTINCTION_COEFF = 0.5;

#define PI 3.14159265358979323846264338327
#define FRONT 0.f
#define INSIDE 1.f
#define BEHIND 2.f
#define MISS 3.f

// ------------------------------------------------------------ //
//                      General Methods                         //
// ------------------------------------------------------------ //

float rand(vec2 co) {
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
}

float saturate(float x) {
    return clamp(x, 0.0, 1.0);
}

float remap(float x, float low1, float high1, float low2, float high2) {
    return low2 + (x - low1) * (high2 - low2) / (high1 - low1);
}

float linearize_depth(float d, float zNear, float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));
}

vec3 luminance(vec3 col) {
    float R = col.r;
    float G = col.g;
    float B = col.b;
    return vec3(0.299 * R + 0.587 * G + 0.114 * B);
}

// ------------------------------------------------------------ //
//                  Sphere Intersection Methods                 //
// ------------------------------------------------------------ //

bool solveQuadratic(float a, float b, float c, float d, out float t0, out float t1) {
    if (d > 0.0) {
        t0 = max((-b - sqrt(d)) / (2.0 * a), 0.0);
        t1 = (-b + sqrt(d)) / (2.0 * a);
        return true;
    }
    else {
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
    float d = (b * b) - 4.0 * a * c;

    return solveQuadratic(a, b, c, d, t0, t1);
}

float getIntersectionPosition(vec3 rayPos, vec3 rayDir, vec3 center, float radius, out float t0, out float t1) {
    if (sphereIntersect(center, radius, rayPos, rayDir, t0, t1)) {
        if (t1 > 0.f) {
            if (t0 > 0.f) {
                return FRONT;
            }
            else {
                return INSIDE;
            }
        }
        else {
            return BEHIND;
        }
    }
    else if (t1 == t0) {
        return BEHIND;
    }
    else {
        return MISS;
    }
}

vec2 cloudShellIntersection(vec3 rayPos, vec3 rayDir) {

    float t0, t1;
    float hit_outerShell = getIntersectionPosition(rayPos, rayDir, cloudCenter, cloudRadius + thickness, t0, t1);

    if (hit_outerShell != MISS || hit_outerShell != BEHIND) {
        if (hit_outerShell == INSIDE) {
            return vec2(0.f, t1);
        }
        else if (hit_outerShell == FRONT) {
            return vec2(t0, t1 - t0);
        }
    }
    else {
        return vec2(0.f, 0.f);
    }
}

// ------------------------------------------------------------ //
// SDF Primitive Combinations
// source: https://www.iquilezles.org/www/articles/distfunctions/distfunctions.htm
// ------------------------------------------------------------ //

float sdSphere(vec3 p, float s) {
    return length(p) - s;
}

float opUnion(float d1, float d2) { return min(d1, d2); }

float opSubtraction(float d1, float d2) { return max(-d1, d2); }

float opIntersection(float d1, float d2) { return max(d1, d2); }

float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) - k * h * (1.0 - h);
}

float opSmoothSubtraction(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5 * (d2 + d1) / k, 0.0, 1.0);
    return mix(d2, -d1, h) + k * h * (1.0 - h);
}

float opSmoothIntersection(float d1, float d2, float k) {
    float h = clamp(0.5 - 0.5 * (d2 - d1) / k, 0.0, 1.0);
    return mix(d2, d1, h) + k * h * (1.0 - h);
}

// ------------------------------------------------------------ //
//                  Volumetric Cloud Methods                    //
// ------------------------------------------------------------ //

float beer(float density) { return exp(-density * EXTINCTION_COEFF); }

bool cloudDensity(vec3 p, out float density, out float dist) {
    p = vec3(t * vec4(p, 1.0));

    float innerShell = sdSphere(p, cloudRadius);
    float outerShell = sdSphere(p, cloudRadius + thickness);
    float cloudShell = opSubtraction(innerShell, outerShell);

    if (cloudShell < 0.f) {

        float minHeight = (normalize(p) * cloudRadius).y;
        float maxHeight = (normalize(p) * (cloudRadius + thickness)).y;

        float p_h = remap(p.y, minHeight, maxHeight, 0.f, 1.f);

        float SR_b = saturate(remap(p_h, 0.0, cloudBottomRoundness, 0.f, 1.f));
        float SR_t = saturate(remap(p_h, 1.0 * cloudTopRoundness, 1.0, 1.f, 0.f));

        float DR_b = p_h * saturate(remap(p_h, 0.0, bottomDensity, 0.f, 1.f));
        float DR_t = saturate(remap(p_h, topDensity, 1.f, 1.f, 0.f));

        float SA = SR_b * SR_t;
        float DA = DR_b * DR_t * 2;

        vec3 sphereNormal = normalize(p - cloudCenter);
        float u = atan(sphereNormal.x, sphereNormal.z) / (2 * PI) + 0.5;
        float v = asin(sphereNormal.y) / PI + 0.5;

        vec4 cns = texture(coverageTex, vec2(u - (time * 0.05f), v) * coverageScale);
        vec4 sns = texture(noisetex, p * noiseScale + (time * 1.f));
        vec4 dns = texture(detailNoiseTex, p * detailScale + (time * 10.f));

        float DN = dns.r * 0.625 + dns.g * 0.25 + dns.b * 0.125;
        float SN = sns.g * 0.625 + sns.b * 0.25 + sns.a * 0.125;

        float coverageNoise = remap(cns.x * coverageIntensity, 0.f, 1.f, -1.f, 1.f);
        float shapeNoise = remap(sns.r, SN - 1.f, 1, 0.f, 1.f) * noiseIntensity;

        shapeNoise = saturate(remap(shapeNoise * SA, 1 - cns.x, 1, 0, 1));

        float detailNoise = 0.35 * exp(-cns.x * 0.75) * mix(DN, 1.f - DN, saturate(p_h * 3.f)) * detailIntensity;

        float noise = saturate(remap(shapeNoise, detailNoise, 1.f, 0.f, 1.f));

        cloudShell = opSubtraction(coverageNoise, cloudShell);
        cloudShell = noise * cloudShell;

        if (cloudShell < 0.f) {
            float sdfMultiplier = min(abs(cloudShell), 1.f) * DA;
            density = sdfMultiplier;
            dist = cloudShell;
            return true;
        }
        else {
            density = 0.f;
            dist = cloudShell;
            return false;
        }
    }
    else {
        density = 0.f;
        return false;
    }
}

float cloudDepth(vec3 rayDir, vec3 rayPos) {

    float depth = 0.f;
    float transmittance = 1.f;
    float totalDist = 0.f;
    float depthCount = 0.f;

    vec2 sphereInfo = cloudShellIntersection(rayPos, rayDir);

    float start = sphereInfo.x;
    float rayLength = min(sphereInfo.y, 250);

    float stepSizeOrig = min(rayLength / float(MAX_CLOUD_STEPS), 1.f);

    //stepSizeOrig += (rayLength) * 0.001;
    float stepSize = stepSizeOrig;

    vec3 currentPos;
    bool exit = false;
    //while(totalDist < rayLength && totalDist+start < worldDepthLinear){
    while (totalDist < rayLength) {

        stepSize = stepSizeOrig;
        currentPos = rayPos + (totalDist + start) * rayDir;

        if (totalDist + stepSize > rayLength || transmittance < 1e-4) {
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
            transmittance *= transmittance_i;

            depth += (start + totalDist) * transmittance;
            depthCount += transmittance;
        }
        if (exit) break;
    }
    if (depthCount != 0.f) depth /= depthCount;

    depth = depth * (1.f - transmittance);// +worldDepthLinear * (transmittance);
    return depth;
}

void main() {

    vec2 uv = (gl_FragCoord.xy / iResolution.xy) - vec2(0.5);
    uv.x *= iResolution.x / iResolution.y;

    vec3 rayDir = mat3(camRight, camUp, camDir) * normalize(vec3(uv, 1.0));
    vec3 rayPos = camPos;

    float depth = cloudDepth(rayDir, rayPos)*0.0001;

	float dx = dFdx(depth);
	float dy = dFdy(depth);
	float moment2 = depth * depth  + 0.25 * (dx*dx + dy*dy);

	FragColor = vec4(depth, moment2, 0.f, 0.f);
}