#ifndef RENDER_H
#define RENDER_H

#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Clouds.h"
#include "FrameBuffer.h"
#include <cmath>

class Clouds;

class Render {

public:
    unsigned int shadowMapTempTarget;
    Texture* colorBuffer;
    Texture* depthBuffer;
    Texture* blueNoise;

    FrameBuffer* fbo;
    FrameBuffer* fbo_depth;
    FrameBuffer* fbo_sdwmap_tmptarget;
    FrameBuffer* fbo_hdr_tmptarget;

    FrameBuffer* fbo_luminance_256;
    FrameBuffer* fbo_luminance_128;
    FrameBuffer* fbo_luminance_64;
    FrameBuffer* fbo_luminance_32;
    FrameBuffer* fbo_luminance_16;
    FrameBuffer* fbo_luminance_4;
    FrameBuffer* fbo_luminance_1;

    Shader depthShader;
    Shader PBRShader;
    Shader _filter;
    Shader hdrShader;
    Shader luminanceShader;

    PBRobj* screen = new PBRobj("../models/screen.obj", glm::vec3(0.f), 1.f);

    int SCR_WIDTH;
    int SCR_HEIGHT;

    Camera *camera;
    Clouds *clouds;

    Render(Camera* _camera, int _SCR_WIDTH = 1000, int _SCR_HEIGHT = 1000);
	
    void Draw(vector<PBRobj*> objects, Light light, Camera camera);
    void setScreenSize(int _SCR_WIDTH, int _SCR_HEIGHT);
    void setDepthTex(int WIDTH, int HEIGHT);
    void setFrameTex(int WIDTH, int HEIGHT);
    void setAtmosphere(Clouds* _clouds);

private:

    unsigned int frameBuffer;
    unsigned int frameBuffer2;

    void sendDepthUniforms(PBRobj* obj, Camera camera);
    void sendPBRUniforms(PBRobj* obj, Light light, Camera camera, Shader shader);
    void setColorFBO();
    void setDepthFBO();
    void initFBOs();
    void applyFilter(Shader filter, FrameBuffer* src, FrameBuffer* dst);
    void computeSceneLuminance();
};
#endif