#ifndef CLOUD_H
#define CLOUD_H

//#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "RenderContext.h"
#include "Light.h"
#include "Texture.h"
#include "FrameBuffer.h"

class RenderContext;
class Light;

class Clouds 
{
public:

    /*    float density = 3.f;
    float topDensity = 0.9;
    float bottomDensity = 0.1552;

    float coverageIntensity = 4.0;
    float noiseIntensity = 0.78;
    float detailIntensity = 1.0;

    float coverageScale = 0.0006;
    float noiseScale = 0.01;
    float detailScale = 0.07;

    float thickness = 35.f;
    float cloudTopRoundness = 0.1724f;
    float cloudBottomRoundness = 0.f;

    float attinuationScalar = 1.f;
    float attinuationClamp = 2.f;

    float planetRadius = 6000;
    float atmosphereRadius = 6250;
    float scaleHeight_rayleigh = 20.f;
    float scaleHeight_mie = 4.0f;

    float ray_intensity = 200.f;
    float mie_intensity = 50.f;
    float absorption_intensity = 25.f;

    float ap_cloud_intensity = 1200.0;
    float ap_world_intensity = 60.0;

    float hg_g = -0.02f;
    float silver_intensity = 1.63f;
    float silver_spread = 0.12f;

    float time = 1.f;

    vec3 ambientColor = vec3(10.f / 255.f, 9.f / 255.f, 14.f / 255.f);

    float cloudRadius = 2503;
    glm::vec3 cloudCenter = vec3(0.f,3575.f,0.f);*/

    Texture* noiseTex;
    Texture* detailNoiseTex;
    Texture* coverageTex;
    Texture* blueNoise;

    float density = 1.f;
    float topDensity = 0.f;
    float bottomDensity = 0.f;

    float coverageIntensity = 3.0;
    float noiseIntensity = 1.61;
    float detailIntensity = 1.2;

    float coverageScale = 1.0;
    float noiseScale = 0.006;
    float detailScale = 0.025;

    float thickness = 20.f;
    float cloudTopRoundness = 0.1295;
    float cloudBottomRoundness = 0.3367f;

    float attinuationScalar = 1.f;
    float attinuationClamp = 2.f;

    float planetRadius = 1000;
    float atmosphereRadius = 1200;
    float scaleHeight_rayleigh = 10.f;
    float scaleHeight_mie = 4.0f;

    float ray_intensity = 500.f;
    float mie_intensity = 100.f;
    float absorption_intensity = 10.f;

    float ap_cloud_intensity = 1200.0;
    float ap_world_intensity = 60.0;

    float hg_g = -0.02f;
    float silver_intensity = 1.13f;
    float silver_spread = 0.12f;

    float time = 1.f;

    vec3 ambientColor = vec3(27.f / 255.f, 48.f / 255.f, 69.f / 255.f);

    float cloudRadius = 1000;
    glm::vec3 cloudCenter = vec3(0.f,0.f,0.f);
    Model model;
    glm::mat4 model_mat;
    glm::mat4 cloud_mat;

    RenderContext* renderContext;

    FrameBuffer* fb;

    unsigned int frameTex;
    unsigned int depthTex;

    Clouds(RenderContext *_renderContext);
    void Draw(Light _light);
    void SetRenderContext(RenderContext *_renderContext);

private:

    Shader shader;

    unsigned int frameBuffer;
    unsigned int depthBuffer;

    void sendUniforms(Light light);
    void sendDepthUniforms(Light light);
    void setColorFBO();
    void setDepthFBO();
    void setDepthTex();
    void setFrameTex();
    void initFBO();
};
#endif