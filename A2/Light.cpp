
#include "Light.h"

void Light::drawShadowMap(vector<PBRobj*> objects) {
	fbo->bind();
	glCullFace(GL_FRONT);
	/*for (auto obj : objects) {
		shdwMapShader.use();
		sendUniforms(obj);
		obj->model.Draw(shdwMapShader);
	}*/	
	if (cloudShadows) {
		cloudShadowMapShader.use();
		sendUniforms_cloudShadows(clouds->model_mat);
		clouds->model.Draw(cloudShadowMapShader);
	}
	glCullFace(GL_BACK);
}

void Light::sendUniforms_cloudShadows(mat4 model_mat) {
	if (clouds != nullptr) {

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, clouds->noiseTex->id);
		cloudShadowMapShader.setInt("noisetex", 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_3D, clouds->detailNoiseTex->id);
		cloudShadowMapShader.setInt("detailNoiseTex", 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, clouds->coverageTex->id);
		cloudShadowMapShader.setInt("coverageTex", 2);

		cloudShadowMapShader.setMat4("model", model_mat);
		cloudShadowMapShader.setMat4("LSM", getLSMatrix());
		
		updateLightVectors();

		cloudShadowMapShader.setVec3("camPos", position);
		cloudShadowMapShader.setVec3("camDir", -getDir());
		cloudShadowMapShader.setVec3("camUp", Up);
		cloudShadowMapShader.setVec3("camRight", Right);
		
		cloudShadowMapShader.setVec2("iResolution", vec2(SHADOW_WIDTH, SHADOW_HEIGHT));

		cloudShadowMapShader.setFloat("density", clouds->density);
		cloudShadowMapShader.setFloat("bottomDensity", clouds->bottomDensity);
		cloudShadowMapShader.setFloat("topDensity", clouds->topDensity);

		cloudShadowMapShader.setFloat("coverageIntensity", clouds->coverageIntensity);
		cloudShadowMapShader.setFloat("noiseIntensity", clouds->noiseIntensity);
		cloudShadowMapShader.setFloat("detailIntensity", clouds->detailIntensity);

		cloudShadowMapShader.setFloat("coverageScale", clouds->coverageScale);
		cloudShadowMapShader.setFloat("noiseScale", clouds->noiseScale);
		cloudShadowMapShader.setFloat("detailScale", clouds->detailScale);

		cloudShadowMapShader.setFloat("thickness", clouds->thickness);

		cloudShadowMapShader.setFloat("cloudTopRoundness", clouds->cloudTopRoundness);
		cloudShadowMapShader.setFloat("cloudBottomRoundness", clouds->cloudBottomRoundness);

		cloudShadowMapShader.setFloat("cloudRadius", clouds->cloudRadius);
		cloudShadowMapShader.setVec3("cloudCenter", clouds->cloudCenter);

		mat4 cloud_mat = inverse(mat4(1.0, 0.0, 0.0, 0.0,  // 1. column
			0.0, 1.0, 0.0, 0.0,  // 2. column
			0.0, 0.0, 1.0, 0.0,  // 3. column
			clouds->cloudCenter.x, clouds->cloudCenter.y, clouds->cloudCenter.z, 1.0)); // 4. column

		cloudShadowMapShader.setMat4("t", cloud_mat);
		cloudShadowMapShader.setFloat("time", clouds->time);
	}
}