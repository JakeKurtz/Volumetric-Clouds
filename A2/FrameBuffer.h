#ifndef FBO_H
#define FBO_H

#include <glad/glad.h> 
#include <glm/glm.hpp>

#include <string>
#include <iostream>
#include <vector>

#include "Attachment.h"

enum aType
{ 
	Color0,
	Color1,
	Color2,
	Color3,
	Color4,
	Color5,
	Color6,
	Color7,
	Color8,
	Color9,
	Color10,
	Color11,
	Color12,
	Color13,
	Color14,
	Color15,
	Color16,
	Color17,
	Color18,
	Color19,
	Color20,
	Color21,
	Color22,
	Color23,
	Color24,
	Color25,
	Color26,
	Color27,
	Color28,
	Color29,
	Color30,
	Color31,
	Depth,
	Stencil,
	Main
};

class FrameBuffer {

public:
	unsigned int id;
	attachment* depth_attachment;
	attachment* stencil_attachment;
	attachment* rbo_attachment;
	std::vector<attachment*> color_attachments;

	int width;
	int height;

	int glClearValue = 1;

	FrameBuffer(int _width, int _height);
	void bind(aType a);
	void bind(aType a, int _width, int _height);
	void bind();
	void bind(int _width, int _height);
	int attachColorBuffer(GLenum attachment, GLint internalformat, GLenum format, GLenum type);
	int attachDepthBuffer(GLint internalformat, GLenum format, GLenum type);
	int attachStencilBuffer(GLint internalformat, GLenum format, GLenum type);
	int attachRenderBuffer();

private:

};
#endif