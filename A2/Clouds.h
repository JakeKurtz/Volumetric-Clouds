#ifndef CLOUD_H
#define CLOUD_H

#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"
#include "RenderContext.h"
#include "Texture.h"
#include "FrameBUffer.h"

class RenderContext;

class Clouds 
{
public:
    float density = 0.1;
    float topDensity = 0.9;
    float bottomDensity = 0.0;

    float coverageIntensity = 3.0;
    float noiseIntensity = 2.7;
    float detailIntensity = 4.0;

    float coverageScale = 0.001;
    float noiseScale = 0.010;
    float detailScale = 0.035;

    float thickness = 30.f;
    float cloudTopRoundness = 0.f;
    float cloudBottomRoundness = 0.f;

    float attinuationScalar = 2.f;
    float attinuationClamp = 0.258f;

    float planetRadius = 6000;
    float atmosphereRadius = 10000;
    float scaleHeight_rayleigh = 0.05267f;
    float scaleHeight_mie = 0.25f;

    float ray_intensity = 50.f;
    float mie_intensity = 1.f;
    float absorption_intensity = 5.f;

    float ap_cloud_intensity = 1200.0;
    float ap_world_intensity = 60.0;

    float hg_g = -0.02f;
    float silver_intensity = 0.4f;
    float silver_spread = 0.11f;

    float time = 1.f;

    vec3 ambientColor = vec3(50.f / 255.f, 61.f / 255.f, 76.f / 255.f);

    float cloudRadius = 5000.f;
    glm::vec3 cloudCenter = vec3(0.f,1080.f,0.f);
    Model model;
    glm::mat4 model_mat;

    RenderContext* renderContext;

    FrameBuffer* fb;

    unsigned int frameTex;
    unsigned int depthTex;

    Clouds(RenderContext* _renderContext);
    void Draw(Light _light, Camera camera);
    void SetRenderContext(RenderContext* _renderContext);

private:

    Shader shader;

    Texture* noiseTex;
    Texture* detailNoiseTex;
    Texture* coverageTex;
    Texture* blueNoise;

    unsigned int frameBuffer;
    unsigned int depthBuffer;

    void sendUniforms(Light light, Camera camera);
    void sendDepthUniforms(Light light, Camera camera);
    void setColorFBO();
    void setDepthFBO();
    void setDepthTex();
    void setFrameTex();
    void initFBO();
};
#endif