#ifndef ATTACHMENT_H
#define ATTACHMENT_H

#include <glad\glad.h>

struct attachment {

    unsigned int id;
    int width;
    int height;

    GLenum attachmentType;
    GLint internalformat;
    GLenum format;
    GLenum type;

    attachment(unsigned int _id, int _width, int _height, GLenum _attachmentType, GLint _internalformat, GLenum _format, GLenum _type) {
        id = _id;
        width = _width;
        height = _height;
        attachmentType = _attachmentType;
        internalformat = _internalformat;
        format = _format;
        type = _type;
    }

    void bind() {
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, width, height, 0, format, type, NULL);
    }
};

#endif
