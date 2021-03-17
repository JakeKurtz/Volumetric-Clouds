#ifndef RENDER_H
#define RENDER_H

#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"
#include "Clouds.h"

class Clouds;

class Render {

public:
    unsigned int frameTex;
    unsigned int shadowMapTempTarget;
    unsigned int depthTex;
    Texture* blueNoise;

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
    Shader depthShader;
    Shader PBRShader;
    Shader _filter;

    unsigned int frameBuffer;
    unsigned int depthBuffer;
    unsigned int frameBuffer2;

    void sendDepthUniforms(PBRobj* obj, Camera camera);
    void sendPBRUniforms(PBRobj* obj, Light light, Camera camera, Shader shader);
    void setColorFBO();
    void setDepthFBO();
    void initFBOs();
    void applyFilter(Shader filter, unsigned int source, unsigned int dest, Light light);
};
#endif