#include "Clouds.h"

Clouds::Clouds(RenderContext* _renderContext) : 
    model("../models/sphere.obj"), 
    shader("../shaders/sky_vs.glsl", "../shaders/sky_fs.glsl")
{
    noiseTex = new Texture("noiseTex128.png", "../textures/", "noise", D3D);
    detailNoiseTex = new Texture("noiseTex32.png", "../textures/", "noise", D3D);
    coverageTex = new Texture("coverage1024.png", "../textures/", "noise");
    blueNoise = new Texture("HDR_L_0.png", "../textures/", "noise");

    renderContext = _renderContext;

    initFBO();

    model_mat = glm::translate(mat4(), cloudCenter);
    model_mat = glm::scale(model_mat, vec3(6250 + 6000 + 20));
}
void Clouds::Draw(Light _light, Camera camera) {

    fb->bind();

    shader.use();
    sendUniforms(_light, camera);
    model.Draw(shader);

}
void Clouds::SetRenderContext(RenderContext* _renderContext) {
    renderContext = _renderContext;
}
void Clouds::sendUniforms(Light light, Camera camera) {

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, noiseTex->id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, detailNoiseTex->id);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, coverageTex->id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, blueNoise->id);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, renderContext->fbo->color_attachments[0]->id);

    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, renderContext->fbo_depth->depth_attachment->id);

    shader.setMat4("model", model_mat);
    shader.setMat4("projection", camera.GetProjMatrix(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT));
    shader.setMat4("view", camera.GetViewMatrix());

    shader.setVec3("camPos", camera.Position);
    shader.setVec3("camDir", camera.Front);
    shader.setVec3("camUp", camera.Up);
    shader.setVec3("camRight", camera.Right);

    shader.setVec2("iResolution", vec2(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT));

    shader.setVec3("planetCenter", vec3(0, 0, 0));

    shader.setFloat("atmosphereRadius", atmosphereRadius);
    shader.setFloat("planetRadius", planetRadius);
    shader.setFloat("scaleHeight_rayleigh", scaleHeight_rayleigh);
    shader.setFloat("scaleHeight_mie", scaleHeight_mie);

    shader.setFloat("ray_intensity", ray_intensity);
    shader.setFloat("mie_intensity", mie_intensity);
    shader.setFloat("absorption_intensity", absorption_intensity);

    shader.setFloat("ap_world_intensity", ap_world_intensity);
    shader.setFloat("ap_cloud_intensity", ap_cloud_intensity);

    shader.setFloat("lightIntensity", light.intensity);
    shader.setVec3("lightDir", light.getDir());
    shader.setVec3("lightColor", light.color);

    shader.setVec3("ambientColor", ambientColor);

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

    shader.setFloat("attinuationScalar", attinuationScalar);
    shader.setFloat("attinuationClamp", attinuationClamp);

    shader.setFloat("cloudRadius", cloudRadius);
    shader.setVec3("cloudCenter", cloudCenter);

    shader.setFloat("time", time);

    shader.setInt("noisetex", 0);
    shader.setInt("detailNoiseTex", 1);
    shader.setInt("coverageTex", 2);
    shader.setInt("blueNoise", 3);
    shader.setInt("screenTexture", 4);
    shader.setInt("cameraDepthTexture", 5);
}
void Clouds::sendDepthUniforms(Light light, Camera camera) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, noiseTex->id);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_3D, detailNoiseTex->id);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, coverageTex->id);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, blueNoise->id);

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, renderContext->fbo_depth->depth_attachment->id);

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
void Clouds::initFBO() {
    fb = new FrameBuffer(renderContext->SCR_WIDTH, renderContext->SCR_HEIGHT);
    fb->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    fb->attachDepthBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
}