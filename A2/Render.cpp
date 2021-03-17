#include "Render.h"

Render::Render(Camera* _camera, int _SCR_WIDTH, int _SCR_HEIGHT) : depthShader("../shaders/cameraDepth_vs.glsl", "../shaders/cameraDepth_fs.glsl"), PBRShader("../shaders/earth_vs.glsl", "../shaders/earth_fs.glsl"), _filter("../shaders/gauss_filter_vs.glsl", "../shaders/gauss_filter_fs.glsl") {
    SCR_WIDTH = _SCR_WIDTH;
    SCR_HEIGHT = _SCR_HEIGHT;

    camera = _camera;
    blueNoise = new Texture("LDR_RG01_0.png", "../textures/", "noise");

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

    light.drawShadowMap(objects);

    applyFilter(_filter, light.shadowTex, shadowMapTempTarget, light);

    setColorFBO();
    for (auto obj : objects) {
        obj->shader.use();
        sendPBRUniforms(obj, light, camera, obj->shader);
        obj->model.Draw(obj->shader);
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
void Render::sendPBRUniforms(PBRobj* obj, Light light, Camera camera, Shader shader) {

    auto textures = obj->model.textures_loaded;
    auto texture_diffuse = -1;
    if (textures.size() > 1) texture_diffuse = textures[0].id;
    //auto texture_specular = textures[1].id;
    //auto texture_normal = textures[2].id;
    //auto texture_height = textures[3].id;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_diffuse);
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

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, blueNoise->id);

    shader.setMat4("model", obj->model_mat);
    shader.setMat4("projection", camera.GetProjMatrix(SCR_WIDTH, SCR_HEIGHT));
    shader.setMat4("view", camera.GetViewMatrix());

    shader.setVec3("camPos", camera.Position);
    shader.setMat4("LSM", light.getLSMatrix());
    shader.setVec3("lightDir", light.getDir());
    shader.setVec3("lightColor", light.color);

    shader.setFloat("lightIntensity", light.intensity);
    //shader.setVec3("ambientColor", ambientColor);

    if (clouds != nullptr) {
        shader.setVec3("planetCenter", vec3(0, 0, 0));
        shader.setFloat("atmosphereRadius", clouds->atmosphereRadius);
        shader.setFloat("planetRadius", clouds->planetRadius);
        shader.setFloat("scaleHeight_rayleigh", clouds->scaleHeight_rayleigh);
        shader.setFloat("scaleHeight_mie", clouds->scaleHeight_mie);
        shader.setFloat("ray_intensity", clouds->ray_intensity);
        shader.setFloat("mie_intensity", clouds->mie_intensity);
        shader.setFloat("absorption_intensity", clouds->absorption_intensity);
    }

    shader.setInt("texture_diffuse", 0);
    shader.setInt("shadowMap", 1);
    shader.setInt("blueNoise", 2);
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

    // ---- test ---- //
    glGenFramebuffers(1, &frameBuffer2);
    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer2);

    glGenTextures(1, &shadowMapTempTarget);
    glBindTexture(GL_TEXTURE_2D, shadowMapTempTarget);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, 4096, 4096, 0, GL_RG, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMapTempTarget, 0);

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
void Render::applyFilter(Shader filter, unsigned int source, unsigned int dest, Light light) {
    //assert(source != dest);
    //if (dest == 0)
    float blurAmount = 0.01f;
    PBRobj screen("../models/screen.obj", glm::vec3(0.f), 1.f);

    glDisable(GL_DEPTH_TEST);

    for (int i = 0; i < 3; i++) {

        glViewport(0, 0, 4096, 4096);
        glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer2);

        glClear(GL_COLOR_BUFFER_BIT);

        filter.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, source);

        filter.setBool("horizontal", true);
        filter.setInt("image", 0);

        screen.model.Draw(filter);

        // ----------------------------------------------- //
        glViewport(0, 0, 4096, 4096);
        glBindFramebuffer(GL_FRAMEBUFFER, light.shadowFBO);

        glClear(GL_COLOR_BUFFER_BIT);

        filter.use();

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, dest);

        filter.setBool("horizontal", false);
        filter.setInt("image", 0);

        screen.model.Draw(filter);
    }

    glEnable(GL_DEPTH_TEST);
}