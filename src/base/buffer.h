#pragma once

#include "gl_utility.h"

class Buffer {
public:
    Buffer() = default;
    explicit Buffer(GLenum target);
    Buffer(Buffer&& rhs) noexcept;
    Buffer& operator=(Buffer&& rhs) noexcept;
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    virtual ~Buffer();

    void bind() const;
    void unbind() const;
    void setData(GLsizeiptr size, const void* data, GLenum usage) const;
    void setSubData(GLintptr offset, GLsizeiptr size, const void* data) const;

    GLuint getHandle() const;
    GLenum getTarget() const;

protected:
    void reset(GLenum target = 0);

private:
    GLuint _handle = 0;
    GLenum _target = 0;
};

class VertexBuffer : public Buffer {
public:
    VertexBuffer();
};

class IndexBuffer : public Buffer {
public:
    IndexBuffer();
};
