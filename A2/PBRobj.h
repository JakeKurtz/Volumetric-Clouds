#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Model.h"

using namespace glm;

class PBRobj {

public:

	vec3 position;
	vec3 obj_scale;
	mat4 model_mat = mat4();
	Model model;

	PBRobj(const char* filepath, vec3 _position = vec3(0.f), vec3 _scale = vec3(1.f)) : model(filepath) {
		position = _position;
		obj_scale = _scale;
	}
	PBRobj(const char* filepath, vec3 _position = vec3(0.f), float _scale = 1.f) : model(filepath) {
		position = _position;
		obj_scale = vec3(_scale);
	}

	void updateTRS() {
		model_mat = mat4();
		model_mat = glm::translate(model_mat, position);
		//model_mat = glm::rotate(model_mat, position);
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
};