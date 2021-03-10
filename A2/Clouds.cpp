#ifndef CLOUD_H
#define CLOUD_H

#include "Clouds.h"
#include "Render.h"
#include "Texture.h"

Clouds::Clouds(Render* _renderContext) : model("../models/sphere.obj"), shader("../shaders/sky_vs.glsl", "../shaders/sky_fs.glsl"), depthShader("../shaders/cloud_vs.glsl", "../shaders/cloud_fs.glsl") {
    noiseTex = TextureFromFile("noiseTex128.png", "../textures/", D3D, false);
    detailNoiseTex = TextureFromFile("noiseTex32.png", "../textures/", D3D, false);
    coverageTex = TextureFromFile("coverage1024.png", "../textures/", D2D, false);
    blueNoise = TextureFromFile("HDR_L_0.png", "../textures/", D2D, false);

    renderContext = _renderContext;

    initFBO();

    model_mat = glm::translate(mat4(), cloudCenter);
    model_mat = glm::scale(model_mat, vec3(6250 + 6000 + 20));
}
void Clouds::Draw(Light _light, Camera camera) {
    setColorFBO();
    shader.use();
    sendUniforms(_light, camera);
    model.Draw(shader);

    //setDepthFBO();
    //depthShader.use();
    //sendDepthUniforms(_light, camera);
    //model.Draw(depthShader);
}
void Clouds::SetRenderContext(Render* _renderContext) {
    renderContext = _renderContext;
}

void Clouds::sendUniforms(Light light, Camera camera) {

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, noiseTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, detailNoiseTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, coverageTex);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, blueNoise);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, renderContext->frameTex);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, renderContext->depthTex);

    shader.setMat4("model", model_mat);
    shader.setMat4("projection", camera.GetProjMatrix(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT));
    shader.setMat4("view", camera.GetViewMatrix());

    shader.setVec3("camPos", camera.Position);
    shader.setVec3("camDir", camera.Front);
    shader.setVec3("camUp", camera.Up);
    shader.setVec3("camRight", camera.Right);

    shader.setFloat("time", 0.f);

    shader.setVec2("iResolution", vec2(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT));

    shader.setVec3("planetCenter", vec3(0, 0, 0));

    shader.setFloat("atmosphereRadius", atmosphereRadius);
    shader.setFloat("planetRadius", planetRadius);
    shader.setFloat("scaleHeight_rayleigh", scaleHeight_rayleigh);
    shader.setFloat("scaleHeight_mie", scaleHeight_mie);

    shader.setFloat("ray_intensity", ray_intensity);
    shader.setFloat("mie_intensity", mie_intensity);
    shader.setFloat("absorption_intensity", absorption_intensity);

    shader.setFloat("lightIntensity", light.intensity);
    shader.setVec3("lightDir", light.getDir());
    shader.setVec3("lightColor", light.color);

    shader.setFloat("g", hg_g);
    shader.setFloat("silver_intensity", silver_intensity);
    shader.setFloat("silver_spread", silver_spread);

    shader.setFloat("density", density);
    shader.setFloat("bottomDensity", bottomDensity);
    shader.setFloat("topDensity", topDensity);

    shader.setFloat("coverageIntensity", coverageIntensity);
    shader.setFloat("noiseIntensity", noiseIntensity);
    shader.setFloat("detailIntensity", detailIntensity);

    shader.setFloat("coverageScale", coverageScale);
    shader.setFloat("noiseScale", noiseScale);
    shader.setFloat("detailScale", detailScale);

    shader.setFloat("thickness", thickness);

    shader.setFloat("cloudTopRoundness", cloudTopRoundness);
    shader.setFloat("cloudBottomRoundness", cloudBottomRoundness);

    shader.setFloat("cloudRadius", cloudRadius);
    shader.setVec3("cloudCenter", cloudCenter);

    shader.setInt("noisetex", 0);
    shader.setInt("detailNoiseTex", 1);
    shader.setInt("coverageTex", 2);
    shader.setInt("blueNoise", 3);
    shader.setInt("screenTexture", 4);
    shader.setInt("cameraDepthTexture", 5);
}
void Clouds::sendDepthUniforms(Light light, Camera camera) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, noiseTex);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, detailNoiseTex);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, coverageTex);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, blueNoise);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, renderContext->depthTex);

    shader.setMat4("model", model_mat);
    shader.setMat4("projection", camera.GetProjMatrix(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT));
    shader.setMat4("view", camera.GetViewMatrix());

    shader.setVec3("camPos", camera.Position);
    shader.setVec3("camDir", camera.Front);
    shader.setVec3("camUp", camera.Up);
    shader.setVec3("camRight", camera.Right);

    //shader.setFloat("time", i);

    shader.setVec2("iResolution", vec2(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT));

    shader.setFloat("density", density);
    shader.setFloat("bottomDensity", bottomDensity);
    shader.setFloat("topDensity", topDensity);

    shader.setFloat("coverageIntensity", coverageIntensity);
    shader.setFloat("noiseIntensity", noiseIntensity);
    shader.setFloat("detailIntensity", detailIntensity);

    shader.setFloat("coverageScale", coverageScale);
    shader.setFloat("noiseScale", noiseScale);
    shader.setFloat("detailScale", detailScale);

    shader.setFloat("thickness", thickness);

    shader.setFloat("cloudTopRoundness", cloudTopRoundness);
    shader.setFloat("cloudBottomRoundness", cloudBottomRoundness);

    shader.setFloat("cloudRadius", cloudRadius);
    shader.setVec3("cloudCenter", cloudCenter);

    shader.setInt("noisetex", 0);
    shader.setInt("detailNoiseTex", 1);
    shader.setInt("coverageTex", 2);
    shader.setInt("blueNoise", 3);
    shader.setInt("cameraDepthTexture", 4);
}
void Clouds::setColorFBO() {
    glViewport(0, 0, renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
};
void Clouds::setDepthFBO() {
    glViewport(0, 0, renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);
    glClear(GL_DEPTH_BUFFER_BIT);
};
void Clouds::setDepthTex() {
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
}
void Clouds::setFrameTex() {
    glBindTexture(GL_TEXTURE_2D, frameTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
}
void Clouds::initFBO() {
    glGenFramebuffers(1, &frameBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

    glGenTextures(1, &frameTex);
    setFrameTex();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, frameTex, 0);

    glGenFramebuffers(1, &depthBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, depthBuffer);

    glGenTextures(1, &depthTex);
    setDepthTex();

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, depthTex, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
#endif 