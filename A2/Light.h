#ifndef LIGHT_H
#define LIGHT_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "PBRobj.h"
#include "FrameBuffer.h"
#include "Clouds.h"
#include "RenderContext.h"
#include "Camera.h"

using namespace glm;

enum light_type { directional = 0, point = 1, spot = 2, area = 3 };

class Clouds;
class RenderContext;

class Light {

public:

	glm::vec3 Up;
	glm::vec3 Right;
	glm::vec3 WorldUp = vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 Target = vec3(0.0f, 0.0f, 0.0f);

	vec3 position;
	vec3 direction = vec3(0.01, 1, 0);
	vec3 color;
	vec3 ambientColor;
	float intensity;
	light_type type;
	bool shadows;
	bool cloudShadows = true;

	Clouds* clouds = nullptr;

	float orthoSize = 2016.f;
	float orthoDepth = 100000.f;
	float lightDist = 15000.f;

	mat4 view_mat;
	mat4 proj_mat;
	mat4 lightSpace_mat;

	FrameBuffer* fbo;

	Light(light_type _type = directional, vec3 _position = vec3(0.f), vec3 _color = vec3(1.f), float _intensity = 10.f, bool shadows = false) :
		shdwMapShader("../shaders/shadowMap_depth_vs.glsl", "../shaders/shadowMap_depth_fs.glsl"), 
		cloudShadowMapShader("../shaders/shadowMap_cloudDepth_vs.glsl", "../shaders/shadowMap_cloudDepth_fs.glsl") 
	{
		position = _position;
		color = _color;
		intensity = _intensity;
		type = _type;

		if (shadows) {
			initShadowMapFBO();
		}
	};

	void drawShadowMap(vector<PBRobj*> objects);
	vec3 getDir() {
		//return normalize(position - vec3(0.f));
		return normalize(direction);
	}
	mat4 getLSMatrix() {
		view_mat = lookAt(position, Target, WorldUp);
		proj_mat = glm::perspective(glm::radians(53.f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, 0.1f, 1000000.f);
		//if (type == directional) proj_mat = ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 0.1f, orthoDepth);
		return proj_mat * view_mat;
	}
	unsigned int getShadowTex() {
		return fbo->color_attachments[0]->id;
	}
	void bindShadowFBO() {
		fbo->bind();
	}
	void enableCloudShadows(Clouds* _clouds) {
		clouds = _clouds;
		cloudShadows = true;
	}

	void updateLightVectors() {

		direction = getDir();

		position = direction * lightDist + vec3(0.f, 0.f, 0.f);

		Right = glm::normalize(glm::cross(-direction, WorldUp));
		Up = glm::normalize(glm::cross(Right, -direction));
	}

private:
	Shader shdwMapShader;
	Shader cloudShadowMapShader;
	const int SHADOW_WIDTH = 4096;
	const int SHADOW_HEIGHT = 4096;

	void initShadowMapFBO() {

		fbo = new FrameBuffer(SHADOW_WIDTH, SHADOW_HEIGHT);
		fbo->attachColorBuffer(GL_COLOR_ATTACHMENT0, GL_RGB32F, GL_RG, GL_FLOAT);
		fbo->attachDepthBuffer(GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);

	};
	void sendUniforms(PBRobj* obj) {
		updateLightVectors();
		shdwMapShader.setMat4("model", obj->model_mat);
		shdwMapShader.setMat4("LSM", getLSMatrix());
	}
	void sendUniforms_cloudShadows(mat4 model_mat);
};
#endif