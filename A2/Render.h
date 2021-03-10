#pragma once
#include "Light.h"
#include "PBRobj.h"
#include "Camera.h"
#include "Shader.h"

class Clouds;

class Render {

public:
    unsigned int frameTex;
    unsigned int depthTex;

    int SCR_WIDTH;
    int SCR_HEIGHT;

    Clouds *clouds;

    Render(int _SCR_WIDTH = 800, int _SCR_HEIGHT = 600);
	
    void Draw(vector<PBRobj*> objects, Light light, Camera camera);
    void setScreenSize(int _SCR_WIDTH, int _SCR_HEIGHT);
    void setDepthTex(int WIDTH, int HEIGHT);
    void setFrameTex(int WIDTH, int HEIGHT);
    void setAtmosphere(Clouds* _clouds);

private:
    Shader depthShader;
    Shader PBRShader;

    unsigned int frameBuffer;
    unsigned int depthBuffer;

    void sendDepthUniforms(PBRobj* obj, Camera camera);
    void sendPBRUniforms(PBRobj* obj, Light light, Camera camera);
    void setColorFBO();
    void setDepthFBO();
    void initFBOs();
};

