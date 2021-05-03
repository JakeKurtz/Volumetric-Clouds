#include "FrameBuffer.h"

FrameBuffer::FrameBuffer(int _width, int _height) { 

    glGenFramebuffers(1, &id);

    width = _width;
    height = _height;
}
void FrameBuffer::bind(int _width, int _height) {
    glViewport(0, 0, _width, _height);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
void FrameBuffer::bind(aType a, int _width, int _height) {

    unsigned int id = -1;

    if (a > Color0 && a < Color31) {
        id = color_attachments[a]->id;
    }
    else if (a == Depth) {
        id = depth_attachment->id;
    }
    else if (a == Stencil) {
        id = stencil_attachment->id;
    }
    else if (a == Main) {
        id = 0;
    }

    glViewport(0, 0, _width, _height);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
void FrameBuffer::bind() {
    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
void FrameBuffer::bind(aType a) {

    unsigned int id = -1;

    if (a > Color0 && a < Color31) {
        id = color_attachments[a]->id;
    }
    else if (a == Depth) {
        id = depth_attachment->id;
    }
    else if (a == Stencil) {
        id = stencil_attachment->id;
    }
    else if (a == Main) {
        id = 0;
    }

    glViewport(0, 0, width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, id);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}
int FrameBuffer::attachColorBuffer(GLenum attachmentType, GLint internalformat, GLenum format, GLenum type) {

    bool failed = false;

    if (attachmentType < 0x8CE0 || attachmentType > 0x8CFF) {
        std::cout << "ERROR::FRAMEBUFFER:: Not a valid color attachment." << std::endl;
        failed = true;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, id);

    unsigned int color_id;
    glGenTextures(1, &color_id);
    glBindTexture(GL_TEXTURE_2D, color_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

    //glGenerateMipmap(GL_TEXTURE_2D);
    glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, color_id, 0);

    attachment* color = new attachment(color_id, width, height, attachmentType, internalformat, format, type);

    color_attachments.push_back(color);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (failed) {
        return 0;
    } else {
        glClearValue = glClearValue | GL_COLOR_BUFFER_BIT;
        return 1;
    }
}
int FrameBuffer::attachDepthBuffer(GLint internalformat, GLenum format, GLenum type) {

    bool failed = false;

    glBindFramebuffer(GL_FRAMEBUFFER, id);

    unsigned int depth_id;
    glGenTextures(1, &depth_id);
    glBindTexture(GL_TEXTURE_2D, depth_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth_id, 0);

    depth_attachment = new attachment(depth_id, width, height, GL_DEPTH_ATTACHMENT, internalformat, format, type);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    if (failed) {
        return 0;
    }
    else {
        glClearValue = glClearValue | GL_DEPTH_BUFFER_BIT;
        return 1;
    }
}
int FrameBuffer::attachStencilBuffer(GLint internalformat, GLenum format, GLenum type) {
    bool failed = false;

    glBindFramebuffer(GL_FRAMEBUFFER, id);

    unsigned int stencil_id;
    glGenTextures(1, &stencil_id);
    glBindTexture(GL_TEXTURE_2D, stencil_id);

    glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_TEXTURE_2D, stencil_id, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    stencil_attachment = new attachment(stencil_id, width, height, GL_STENCIL_ATTACHMENT, internalformat, format, type);

    if (failed) {
        return 0;
    }
    else {
        glClearValue = glClearValue | GL_STENCIL_BUFFER_BIT;
        return 1;
    }
}
int FrameBuffer::attachRenderBuffer() {

    unsigned int rbo;

    glBindFramebuffer(GL_FRAMEBUFFER, id);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    rbo_attachment = new attachment(rbo, width, height, GL_DEPTH_STENCIL_ATTACHMENT, GL_DEPTH24_STENCIL8, GL_DEPTH24_STENCIL8, GL_FLOAT);

    return 0;
}