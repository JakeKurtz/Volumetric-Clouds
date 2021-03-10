#pragma once
#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Model.h"

class Render;

class Clouds 
{
public:
    float density = 14.6;
    float topDensity = 0.01;
    float bottomDensity = 1.0;

    float coverageIntensity = 1.0;
    float noiseIntensity = 1.69;
    float detailIntensity = 2.0;

    float coverageScale = 0.0007;
    float noiseScale = 0.015;
    float detailScale = 0.04;

    float thickness = 30.f;
    float cloudTopRoundness = 0.095f;
    float cloudBottomRoundness = 0.f;

    float planetRadius = 6000;
    float atmosphereRadius = 7000;
    float scaleHeight_rayleigh = 50;
    float scaleHeight_mie = 5;

    float ray_intensity = 250;
    float mie_intensity = 120.0;
    float absorption_intensity = 27.0;

    float hg_g = 0.567f;
    float silver_intensity = 0.6f;
    float silver_spread = 0.392f;

    float cloudRadius = 6030.f;
    glm::vec3 cloudCenter = vec3(0.f);
    Model model;
    glm::mat4 model_mat;

    Render* renderContext;

    unsigned int frameTex;
    unsigned int depthTex;

    Clouds(Render* _renderContext);
    void Draw(Light _light, Camera camera);
    void SetRenderContext(Render* _renderContext);

private:

    Shader shader;
    Shader depthShader;

    unsigned int noiseTex;
    unsigned int detailNoiseTex;
    unsigned int coverageTex;
    unsigned int blueNoise;

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
