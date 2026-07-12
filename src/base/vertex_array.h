#pragma once

#include "gl_utility.h"

class VertexArray {
public:
    VertexArray();
    VertexArray(VertexArray&& rhs) noexcept;
    VertexArray& operator=(VertexArray&& rhs) noexcept;
    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    ~VertexArray();

    void bind() const;
    void unbind() const;
    void enableAttrib(GLuint index) const;
    void setAttribPointer(
        GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
        const void* pointer) const;
    void setAttribIPointer(
        GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) const;
    void setAttribDivisor(GLuint index, GLuint divisor) const;

    GLuint getHandle() const;

private:
    void reset();

    GLuint _handle = 0;
};
