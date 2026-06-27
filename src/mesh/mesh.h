#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "../base/gl_utility.h"
#include "../base/vertex.h"

class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh();

    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;
    uint32_t indexCount() const { return _indexCount; }

private:
    GLuint _vao = 0, _vbo = 0, _ebo = 0;
    uint32_t _indexCount = 0;
};


