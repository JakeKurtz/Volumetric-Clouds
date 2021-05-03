#include "RenderContext.h"

RenderContext::RenderContext(Camera* _camera, int _SCR_WIDTH, int _SCR_HEIGHT) :
    depthShader("../shaders/depth_vs.glsl", "../shaders/depth_fs.glsl"),
    gaussFilter("../shaders/gauss_filter_vs.glsl", "../shaders/gauss_filter_fs.glsl"),
    hdrShader("../shaders/screen_vs.glsl", "../shaders/screen_fs.glsl"),
    luminanceShader("../shaders/luminance_vs.glsl", "../shaders/luminance_fs.glsl"),
    starShader("../shaders/star_vs.glsl", "../shaders/star_fs.glsl"),
    sunShader("../shaders/sun_vs.glsl", "../shaders/sun_fs.glsl")
{
    SCR_WIDTH = _SCR_WIDTH;
    SCR_HEIGHT = _SCR_HEIGHT;

    camera = _camera;
    blueNoise = new Texture("LDR_RG01_0.png", "../textures/", "noise");

    initFBOs();
}

float lerp(float v0, float v1, float t) {
    return (1 - t) * v0 + t * v1;
}

void RenderContext::DrawSky(Light light) {

    // You can do better than this...

    celestial_objs[0]->translate(camera->Position + light.getDir() * 10000.f);
    celestial_objs[1]->translate(camera->Position);

    for (auto obj : celestial_objs) {
        obj->updateTRS();
    }

    //for (int i = 0; i < 3; i++) {
    //    applyFilter(gaussFilter, light.fbo, fbo_sdwmap_tmptarget);
    //}

    fbo_celestial->bind();
    for (auto obj : celestial_objs) {
        obj->shader.use();
        sendPBRUniforms(obj, light, obj->shader);
        obj->model.Draw(obj->shader);
    }
}

void RenderContext::Draw(vector<PBRobj*> objects, Light light) {
    
    for (auto obj : objects) {
        obj->updateTRS();
    }
    
    light.drawShadowMap(objects);

    for (int i = 0; i < 3; i++) {
        applyFilter(gaussFilter, light.fbo, fbo_sdwmap_tmptarget);
    }
    
    fbo->bind();
    for (auto obj : objects) {
        obj->shader.use();
        sendPBRUniforms(obj, light, obj->shader);
        obj->model.Draw(obj->shader);
    }
    
    DrawSky(light);

    if (clouds != nullptr) {

        clouds->Draw(light);

        //meanLuminance(clouds->fb->color_attachments[0]->id);
        applyToneMapping(fbo, Main, clouds->fb->color_attachments[0]->id);
    }
}
void RenderContext::setScreenSize(int _SCR_WIDTH, int _SCR_HEIGHT) {
    SCR_WIDTH = _SCR_WIDTH;
    SCR_HEIGHT = _SCR_HEIGHT;
}
void RenderContext::enableAtmosphere() {

    celestialSphere->setShader(starShader);
    sun->setShader(sunShader);

    celestial_objs.push_back(sun);
    celestial_objs.push_back(celestialSphere);

    clouds = new Clouds(this);;
}

void RenderContext::sendDepthUniforms(PBRobj* obj) {
    depthShader.setMat4("model", obj->model_mat);
    depthShader.setMat4("projection", camera->GetProjMatrix(SCR_WIDTH, SCR_HEIGHT));
    depthShader.setMat4("view", camera->GetViewMatrix());
}
void RenderContext::sendPBRUniforms(PBRobj* obj, Light light, Shader shader) {

    auto textures = obj->model.textures_loaded;
    auto texture_diffuse = -1;
    if (textures.size() > 0) texture_diffuse = textures[0].id;
    //auto texture_specular = textures[1].id;
    auto texture_normal = -1;
    if (textures.size() > 1) texture_normal = textures[1].id;
    //auto texture_height = textures[3].id;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_diffuse);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_normal);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_lights->id);
    /*
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture_specular);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, texture_normal);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, texture_height);
    */
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, light.getShadowTex());

    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, blueNoise->id);

    shader.setMat4("model", obj->model_mat);
    shader.setMat4("projection", camera->GetProjMatrix(SCR_WIDTH, SCR_HEIGHT));
    shader.setMat4("view", camera->GetViewMatrix());

    shader.setVec3("camPos", camera->Position);
    shader.setVec3("camDir", camera->Front);
    shader.setVec3("camUp", camera->Up);
    shader.setVec3("camRight", camera->Right);

    shader.setVec2("iResolution", vec2(SCR_WIDTH, SCR_HEIGHT));

    light.updateLightVectors();

    shader.setMat4("LSM", light.getLSMatrix());
    shader.setVec3("lightDir", light.getDir());
    shader.setVec3("lightColor", light.color);
    shader.setVec3("ambientColor", ambientColor);

    shader.setFloat("lightIntensity", light.intensity);

    shader.setFloat("roughness", obj->roughness);
    shader.setFloat("Ka", obj->Ka);
    shader.setFloat("Ks", obj->Ks);

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
    shader.setInt("texture_normal", 1);
    shader.setInt("texture_lights", 2);
    shader.setInt("shadowMap", 3);
    shader.setInt("blueNoise", 4);
}
void RenderContext::initFBOs() {

    fbo = new FrameBuffer(SCR_WIDTH, SCR_HEIGHT);
    fbo->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    fbo->attachDepthBuffer(GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT, GL_FLOAT);

    fbo_sdwmap_tmptarget = new FrameBuffer(4096, 4096);
    fbo_sdwmap_tmptarget->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RG32F, GL_RG, GL_FLOAT);
    
    fbo_hdr_tmptarget = new FrameBuffer(SCR_WIDTH, SCR_HEIGHT);
    fbo_hdr_tmptarget->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    fbo_luminance_1 = new FrameBuffer(SCR_WIDTH, SCR_HEIGHT);
    fbo_luminance_1->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    fbo_luminance_2 = new FrameBuffer(SCR_WIDTH, SCR_HEIGHT);
    fbo_luminance_2->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);

    fbo_celestial = new FrameBuffer(SCR_WIDTH, SCR_HEIGHT);
    fbo_celestial->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGBA16F, GL_RGBA, GL_FLOAT);
    fbo_celestial->attachDepthBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

}
void RenderContext::applyFilter(Shader filter, FrameBuffer* src, FrameBuffer* dst) {

    glDisable(GL_DEPTH_TEST);

    // ----- First pass ----- //

    dst->bind();
    filter.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src->color_attachments[0]->id);

    float blurAmount = 5.f;

    filter.setBool("horizontal", true);
    filter.setInt("image", 0);
    screen->model.Draw(filter);

    // ----- Second pass ----- //
    
    src->bind();
    filter.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, dst->color_attachments[0]->id);

    filter.setBool("horizontal", false);
    filter.setInt("image", 0);
    screen->model.Draw(filter);

    glEnable(GL_DEPTH_TEST);
}
void RenderContext::meanLuminance(unsigned int texId) {

    glDisable(GL_DEPTH_TEST);

    fbo_luminance_1->bind(SCR_WIDTH, SCR_HEIGHT);

    glBindTexture(GL_TEXTURE_2D, texId);

    luminanceShader.use();
    luminanceShader.setBool("last", false);
    screen->model.Draw(luminanceShader);

    int width = SCR_WIDTH;
    int height = SCR_HEIGHT;
    bool k = false;
    while(width > 1) {

        width = (width / 2) > 1 ? width/2 : 1;
        height = (height / 2) > 1 ? height/2 : 1;

        auto fboPingPong_1 = k ? fbo_luminance_1 : fbo_luminance_2;
        auto fboPingPong_2 = k ? fbo_luminance_2 : fbo_luminance_1;

        glBindTexture(GL_TEXTURE_2D, fboPingPong_1->color_attachments[0]->id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);

        fboPingPong_1->bind(width, height);

        glBindTexture(GL_TEXTURE_2D, fboPingPong_2->color_attachments[0]->id);

        luminanceShader.use();
        luminanceShader.setBool("last", width == 1);
        screen->model.Draw(luminanceShader);
        
        k = !k;
    }

    //GLfloat luminescence[3];
    //glReadPixels(0, 0, 1, 1, GL_RGB, GL_FLOAT, &luminescence);
    //GLfloat lum = 0.2126 * luminescence[0] + 0.7152 * luminescence[1] + 0.0722 * luminescence[2];
    //cout << lum << endl;
    //camera->Exposure = lerp((double)camera->Exposure, 1.f/(2.f*lum), 0.05); // slowly adjust exposure based on average brightness

    glEnable(GL_DEPTH_TEST);
}
void RenderContext::applyToneMapping(FrameBuffer* targetFB, aType targetAttachment, unsigned int texId) {

    glDisable(GL_DEPTH_TEST);

    targetFB->bind(targetAttachment);
    hdrShader.use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
    hdrShader.setFloat("exposure", camera->Exposure);
    //hdrShader.setFloat("lum", lum);
    screen->model.Draw(hdrShader);

    glEnable(GL_DEPTH_TEST);
}