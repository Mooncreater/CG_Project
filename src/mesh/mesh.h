#pragma once

#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "../base/buffer.h"
#include "../base/vertex.h"
#include "../base/vertex_array.h"

class Mesh {
public:
    Mesh() = default;
    Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    ~Mesh() = default;

    Mesh(Mesh&& other) noexcept = default;
    Mesh& operator=(Mesh&& other) noexcept = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    void draw() const;
    const std::vector<Vertex>& vertices() const { return _vertices; }
    const std::vector<uint32_t>& indices() const { return _indices; }
    uint32_t indexCount() const { return (uint32_t)_indices.size(); }
    const VertexArray& vao() const { return _vao; }

private:
    std::vector<Vertex> _vertices;
    std::vector<uint32_t> _indices;
    VertexArray _vao;
    VertexBuffer _vbo;
    IndexBuffer _ebo;
};
