#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PBRobj.h"

using namespace glm;

enum light_type { directional = 0, point = 1, spot = 2, area = 3 };

class Light {

public:
	vec3 position;
	vec3 direction;// = vec3(0, 1, 0);
	vec3 color;
	float intensity;
	light_type type;
	bool shadows;

	mat4 view_mat;
	mat4 proj_mat;
	mat4 lightSpace_mat;

	unsigned int shadowTex;

	Light(light_type _type = directional, vec3 _position = vec3(0.f), vec3 _color = vec3(1.f), float _intensity = 1.f, bool shadows = false) : shdwMapShader("../shaders/lightDepth_vs.glsl", "../shaders/lightDepth_fs.glsl"){
		position = _position;
		color = _color;
		intensity = _intensity;
		type = _type;

		if (shadows) {
			initShadowMapFBO();
		}
	};

	void drawShadowMap(vector<PBRobj> objects) {
		setShadowMapFBO();
		glCullFace(GL_FRONT);
		for (auto obj : objects) {
			shdwMapShader.use();
			sendUniforms(obj);
			obj.model.Draw(shdwMapShader);
		}
		glCullFace(GL_BACK);
	}
	vec3 getDir() {
		//if (type == directional) {
		return normalize(direction);
		//}
	}
	mat4 getLSMatrix() {
		view_mat = lookAt(position, position + normalize(direction), vec3(0.0f, 1.0f, 0.0f));
		if (type == directional) proj_mat = ortho(-500.f, 500.f, -500.f, 500.f, 1.f, 500.f);
		return proj_mat * view_mat;
	}

private:
	Shader shdwMapShader;
	unsigned int shadowFBO;
	const int SHADOW_WIDTH = 8192;
	const int SHADOW_HEIGHT = 8192;

	void initShadowMapFBO() {
		glGenFramebuffers(1, &shadowFBO);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

		glGenTextures(1, &shadowTex);
		glBindTexture(GL_TEXTURE_2D, shadowTex);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
		glDrawBuffer(GL_NONE);
		glReadBuffer(GL_NONE);
	};
	void setShadowMapFBO() {
		glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
		glClear(GL_DEPTH_BUFFER_BIT);
	}
	void sendUniforms(PBRobj obj) {
		shdwMapShader.setMat4("model", obj.model_mat);
		shdwMapShader.setMat4("LSM", getLSMatrix());
	}
};
