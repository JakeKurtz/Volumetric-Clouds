#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Clouds.h"
#include "FrameBuffer.h"
#include <cmath>

class Clouds;

class RenderContext {

public:
    unsigned int shadowMapTempTarget;
    Texture* colorBuffer;
    Texture* depthBuffer;
    Texture* blueNoise;

    FrameBuffer* fbo;
    FrameBuffer* fbo_depth;
    FrameBuffer* fbo_sdwmap_tmptarget;
    FrameBuffer* fbo_hdr_tmptarget;

    FrameBuffer* fbo_luminance_1;
    FrameBuffer* fbo_luminance_2;

    Shader depthShader;
    Shader gaussFilter;
    Shader hdrShader;
    Shader luminanceShader;

    PBRobj* screen = new PBRobj("../models/screen.obj", glm::vec3(0.f), 1.f);

    int SCR_WIDTH;
    int SCR_HEIGHT;

    Camera *camera;
    Clouds *clouds = nullptr;

    RenderContext(Camera* _camera, int _SCR_WIDTH = 1000, int _SCR_HEIGHT = 1000);
	
    void Draw(vector<PBRobj*> objects, Light light, Camera camera);
    void setScreenSize(int _SCR_WIDTH, int _SCR_HEIGHT);
    void setDepthTex(int WIDTH, int HEIGHT);
    void setFrameTex(int WIDTH, int HEIGHT);
    void enableAtmosphere();

private:

    unsigned int frameBuffer;
    unsigned int frameBuffer2;

    void sendDepthUniforms(PBRobj* obj, Camera camera);
    void sendPBRUniforms(PBRobj* obj, Light light, Camera camera, Shader shader);
    void setColorFBO();
    void setDepthFBO();
    void initFBOs();
    void applyFilter(Shader filter, FrameBuffer* src, FrameBuffer* dst);
    void meanLuminance(unsigned int texId);
    void applyToneMapping(FrameBuffer* targetFB, aType targetAttachment, unsigned int texId);
};
#endif