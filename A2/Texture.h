#ifndef TEXTURE_H
#define TEXTURE_H

#include <glad/glad.h> 
#include <glm/glm.hpp>
#include <stb_image.h>

#include <string>
#include <iostream>

using namespace std;

enum dimension { D1D, D2D, D3D };

class Texture {
public:
	unsigned int id;
	string type;
	string path;
	dimension dim;

	unsigned int frameBuffer;
	unsigned int frameRbo;

	Texture();
	Texture(int width, int height, GLenum attachment = GL_COLOR_ATTACHMENT0, GLint internalformat = GL_RGB, GLenum format = GL_RGB, GLenum type = GL_FLOAT);
	Texture(const char* path, const string& directory, string type, dimension dim = D2D, bool gamma = false);
};
#endif