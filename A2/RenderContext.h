#ifndef RENDERCONTEXT_H
#define RENDERCONTEXT_H

#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Clouds.h"
#include "FrameBuffer.h"
#include <cmath>
#include "Light.h"

class Clouds;
class Light;

class RenderContext {

public:

    vec3 ambientColor;

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

    FrameBuffer* fbo_celestial;

    Shader depthShader;
    Shader gaussFilter;
    Shader hdrShader;
    Shader luminanceShader;
    Shader starShader;
    Shader sunShader;

    PBRobj* screen = new PBRobj("../models/screen.obj", glm::vec3(0.f), 1.f);
    PBRobj* sun = new PBRobj("../models/sphere.obj", glm::vec3(0.f, 6000.f, 0.f), 250.f);
    PBRobj* celestialSphere = new PBRobj("../models/celestial/celestial.obj", glm::vec3(0.f), 12000.f);

    Texture* texture_lights = new Texture("earth_night_16384x8192.png", "../models/earth", "wut");

    int SCR_WIDTH;
    int SCR_HEIGHT;

    Camera *camera;
    Clouds *clouds = nullptr;

    RenderContext(Camera* _camera, int _SCR_WIDTH = 1000, int _SCR_HEIGHT = 1000);
	
    void Draw(vector<PBRobj*> objects, Light light);
    void DrawSky(Light light);
    void setScreenSize(int _SCR_WIDTH, int _SCR_HEIGHT);
    void setDepthTex(int WIDTH, int HEIGHT);
    void setFrameTex(int WIDTH, int HEIGHT);
    void enableAtmosphere();

private:

    unsigned int frameBuffer;
    unsigned int frameBuffer2;

    vector<PBRobj*> celestial_objs;

    void sendDepthUniforms(PBRobj* obj);
    void sendPBRUniforms(PBRobj* obj, Light light, Shader shader);
    void setColorFBO();
    void setDepthFBO();
    void initFBOs();
    void applyFilter(Shader filter, FrameBuffer* src, FrameBuffer* dst);
    void meanLuminance(unsigned int texId);
    void applyToneMapping(FrameBuffer* targetFB, aType targetAttachment, unsigned int texId);
};
#endif