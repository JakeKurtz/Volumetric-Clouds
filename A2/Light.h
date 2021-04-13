#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PBRobj.h"
#include "FrameBuffer.h"

using namespace glm;

enum light_type { directional = 0, point = 1, spot = 2, area = 3 };

class Light {

public:
	vec3 position;
	vec3 direction;// = vec3(0, 1, 0);
	vec3 color;
	vec3 ambientColor;
	float intensity;
	light_type type;
	bool shadows;

	mat4 view_mat;
	mat4 proj_mat;
	mat4 lightSpace_mat;

	FrameBuffer* fbo;

	Light(light_type _type = directional, vec3 _position = vec3(0.f), vec3 _color = vec3(1.f), float _intensity = 30.f, bool shadows = false) : shdwMapShader("../shaders/shadowMap_depth_vs.glsl", "../shaders/shadowMap_depth_fs.glsl"){
		position = _position;
		color = _color;
		intensity = _intensity;
		type = _type;

		if (shadows) {
			initShadowMapFBO();
		}
	};

	void drawShadowMap(vector<PBRobj*> objects) {
		fbo->bind();
		glCullFace(GL_FRONT);
		for (auto obj : objects) {
			shdwMapShader.use();
			sendUniforms(obj);
			obj->model.Draw(shdwMapShader);
		}
		glCullFace(GL_BACK);
	}
	vec3 getDir() {
		//if (type == directional) {
		return normalize(direction);
		//}
	}
	mat4 getLSMatrix() {

		auto pos = getDir() * 100.f + vec3(0.f, 6000.f, 0.f);

		view_mat = lookAt(pos, vec3(0.f, 6000.f, 0.f), vec3(0.0f, 1.0f, 0.0f));
		if (type == directional) proj_mat = ortho(-100.f, 100.f, -100.f, 100.f, 0.1f, 500.f);
		return proj_mat * view_mat;
	}
	unsigned int getShadowTex() {
		return fbo->color_attachments[0]->id;
	}
	void bindShadowFBO() {
		fbo->bind();
	}

private:
	Shader shdwMapShader;
	const int SHADOW_WIDTH = 4096;
	const int SHADOW_HEIGHT = 4096;

	void initShadowMapFBO() {

		fbo = new FrameBuffer(SHADOW_WIDTH, SHADOW_HEIGHT);
		fbo->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RG32F, GL_RG, GL_FLOAT);
		fbo->attachDepthBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

	};
	void sendUniforms(PBRobj* obj) {
		shdwMapShader.setMat4("model", obj->model_mat);
		shdwMapShader.setMat4("LSM", getLSMatrix());
	}
};
#endif