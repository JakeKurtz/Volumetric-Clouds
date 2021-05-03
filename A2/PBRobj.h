#ifndef PBROBJ_H
#define PBROBJ_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <algorithm>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.h"

using namespace glm;

class PBRobj {

public:

	vec3 position;
	vec3 obj_scale = vec3(1.f);
	vec3 rotAxis = vec3(1.f,0.f,0.f);
	float rotation = 0.f;

	mat4 model_mat = mat4();
	Model model;
	Shader shader;

	float roughness = 0.001;
	float Ka = 0.f;
	float Ks = 0.f;

	PBRobj(const char* filepath, vec3 _position = vec3(0.f), vec3 _scale = vec3(1.f)) : model(filepath), shader("../shaders/pbr_vs.glsl", "../shaders/pbr_fs.glsl") {
		position = _position;
		obj_scale = _scale;
	}
	PBRobj(const char* filepath, vec3 _position = vec3(0.f), float _scale = 1.f) : model(filepath), shader("../shaders/pbr_vs.glsl", "../shaders/pbr_fs.glsl") {
		position = _position;
		obj_scale = vec3(_scale);
	}

	void setShader(Shader _shader) {
		shader = _shader;
	}

	void draw() {
		model.Draw(shader);
	}

	void updateTRS() {
		model_mat = mat4();
		model_mat = glm::translate(model_mat, position);
		model_mat = glm::rotate(model_mat, rotation, rotAxis);
		model_mat = glm::scale(model_mat, vec3(obj_scale));
	}
	void scale(float _scale) {
		obj_scale = vec3(_scale);
	}
	void scale(vec3 _scale) {
		obj_scale = _scale;
	}
	void translate(vec3 _position) {
		position = _position;
	}
	void rotate(vec3 _rotAxis, float _rotation) {
		rotAxis = _rotAxis;
		rotation = _rotation;
	}
};
#endif