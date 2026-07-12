#include "buffer.h"

Buffer::Buffer(GLenum target) : _target(target) {
    glGenBuffers(1, &_handle);
}

Buffer::Buffer(Buffer&& rhs) noexcept : _handle(rhs._handle), _target(rhs._target) {
    rhs._handle = 0;
    rhs._target = 0;
}

Buffer& Buffer::operator=(Buffer&& rhs) noexcept {
    if (this != &rhs) {
        reset();
        _handle = rhs._handle;
        _target = rhs._target;
        rhs._handle = 0;
        rhs._target = 0;
    }
    return *this;
}

Buffer::~Buffer() {
    reset();
}

void Buffer::bind() const {
    glBindBuffer(_target, _handle);
}

void Buffer::unbind() const {
    glBindBuffer(_target, 0);
}

void Buffer::setData(GLsizeiptr size, const void* data, GLenum usage) const {
    glBufferData(_target, size, data, usage);
}

void Buffer::setSubData(GLintptr offset, GLsizeiptr size, const void* data) const {
    glBufferSubData(_target, offset, size, data);
}

GLuint Buffer::getHandle() const {
    return _handle;
}

GLenum Buffer::getTarget() const {
    return _target;
}

void Buffer::reset(GLenum target) {
    if (_handle != 0) {
        glDeleteBuffers(1, &_handle);
    }
    _handle = 0;
    _target = target;
}

VertexBuffer::VertexBuffer() : Buffer(GL_ARRAY_BUFFER) {}

IndexBuffer::IndexBuffer() : Buffer(GL_ELEMENT_ARRAY_BUFFER) {}
