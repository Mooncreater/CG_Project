#include "vertex_array.h"

VertexArray::VertexArray() {
    glGenVertexArrays(1, &_handle);
}

VertexArray::VertexArray(VertexArray&& rhs) noexcept : _handle(rhs._handle) {
    rhs._handle = 0;
}

VertexArray& VertexArray::operator=(VertexArray&& rhs) noexcept {
    if (this != &rhs) {
        reset();
        _handle = rhs._handle;
        rhs._handle = 0;
    }
    return *this;
}

VertexArray::~VertexArray() {
    reset();
}

void VertexArray::bind() const {
    glBindVertexArray(_handle);
}

void VertexArray::unbind() const {
    glBindVertexArray(0);
}

void VertexArray::enableAttrib(GLuint index) const {
    glEnableVertexAttribArray(index);
}

void VertexArray::setAttribPointer(
    GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride,
    const void* pointer) const {
    glVertexAttribPointer(index, size, type, normalized, stride, pointer);
}

void VertexArray::setAttribIPointer(
    GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer) const {
    glVertexAttribIPointer(index, size, type, stride, pointer);
}

void VertexArray::setAttribDivisor(GLuint index, GLuint divisor) const {
    glVertexAttribDivisor(index, divisor);
}

GLuint VertexArray::getHandle() const {
    return _handle;
}

void VertexArray::reset() {
    if (_handle != 0) {
        glDeleteVertexArrays(1, &_handle);
        _handle = 0;
    }
}
