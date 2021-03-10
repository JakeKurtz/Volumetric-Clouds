#ifndef RENDER_H
#define RENDER_H

#include "Render.h"
#include "Clouds.h"

Render::Render(int _SCR_WIDTH, int _SCR_HEIGHT) : depthShader("../shaders/cameraDepth_vs.glsl", "../shaders/cameraDepth_fs.glsl"), PBRShader("../shaders/earth_vs.glsl", "../shaders/earth_fs.glsl") {
    SCR_WIDTH = _SCR_WIDTH;
    SCR_HEIGHT = _SCR_HEIGHT;

    initFBOs();
}

void Render::Draw(vector<PBRobj*> objects, Light light, Camera camera) {

    for (auto obj : objects) {
        obj->updateTRS();
    }

    setDepthFBO();
    for (auto obj : objects) {
        depthShader.use();
        sendDepthUniforms(obj, camera);
        obj->model.Draw(depthShader);
    }

    //light.drawShadowMap(objects);

    setColorFBO();
    for (auto obj : objects) {
        PBRShader.use();
        sendPBRUniforms(obj, light, camera);
        obj->model.Draw(PBRShader);
    }
}
void Render::setScreenSize(int _SCR_WIDTH, int _SCR_HEIGHT) {
    SCR_WIDTH = _SCR_WIDTH;
    SCR_HEIGHT = _SCR_HEIGHT;
}
void Render::setDepthTex(int WIDTH, int HEIGHT) {}
void Render::setFrameTex(int WIDTH, int HEIGHT) {}
void Render::setAtmosphere(Clouds* _clouds) {
    clouds = _clouds;
}

void Render::sendDepthUniforms(PBRobj* obj, Camera camera) {
    depthShader.setMat4("model", obj->model_mat);
    depthShader.setMat4("projection", camera.GetProjMatrix(SCR_WIDTH, SCR_HEIGHT));
    depthShader.setMat4("view", camera.GetViewMatrix());
}
void Render::sendPBRUniforms(PBRobj* obj, Light light, Camera camera) {

    auto textures = obj->model.textures_loaded;
    //auto texture_diffuse = textures[0].id;
    //auto texture_specular = textures[1].id;
    //auto texture_normal = textures[2].id;
    //auto texture_height = textures[3].id;

    //glActiveTexture(GL_TEXTURE0);
    //glBindTexture(GL_TEXTURE_2D, texture_diffuse);
    /*
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_specular);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_normal);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_height);
    */
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, light.shadowTex);

    //glActiveTexture(GL_TEXTURE2);
    //glBindTexture(GL_TEXTURE_2D, blueNoise);

    PBRShader.setMat4("model", obj->model_mat);
    PBRShader.setMat4("projection", camera.GetProjMatrix(SCR_WIDTH, SCR_HEIGHT));
    PBRShader.setMat4("view", camera.GetViewMatrix());

    PBRShader.setVec3("camPos", camera.Position);
    PBRShader.setMat4("LSM", light.getLSMatrix());
    PBRShader.setVec3("lightDir", light.getDir());
    PBRShader.setVec3("lightColor", light.color);

    PBRShader.setFloat("lightIntensity", light.intensity);

    if (clouds != nullptr) {
        PBRShader.setVec3("planetCenter", vec3(0, 0, 0));
        PBRShader.setFloat("atmosphereRadius", clouds->atmosphereRadius);
        PBRShader.setFloat("planetRadius", clouds->planetRadius);
        PBRShader.setFloat("scaleHeight_rayleigh", clouds->scaleHeight_rayleigh);
        PBRShader.setFloat("scaleHeight_mie", clouds->scaleHeight_mie);
        PBRShader.setFloat("ray_intensity", clouds->ray_intensity);
        PBRShader.setFloat("mie_intensity", clouds->mie_intensity);
        PBRShader.setFloat("absorption_intensity", clouds->absorption_intensity);
    }

    PBRShader.setInt("texture_diffuse", 0);
    PBRShader.setInt("shadowMap", 1);
}
void Render::setColorFBO() {
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
};
void Render::setDepthFBO() {
    glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
};
void Render::initFBOs() {
    // ---- COLOR BUFFER ---- //
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenTextures(1, &frameTex);
    glBindTexture(GL_TEXTURE_2D, frameTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameTex, 0);

    //--------------------------------------------------------------------------------------//
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    //--------------------------------------------------------------------------------------//

    // ---- DEPTH BUFFER ---- //
    glGenFramebuffers(1, &depthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);

    glGenTextures(1, &depthTex);
    glBindTexture(GL_TEXTURE_2D, depthTex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif 