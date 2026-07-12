#include "mesh.h"

Mesh::Mesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
    : _vertices(vertices), _indices(indices) {
    _vao.bind();
    _vbo.bind();
    _vbo.setData(
        static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)), vertices.data(), GL_STATIC_DRAW);
    _ebo.bind();
    _ebo.setData(
        static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)), indices.data(), GL_STATIC_DRAW);

    // position: location 0
    _vao.setAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    _vao.enableAttrib(0);
    // normal: location 1
    _vao.setAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    _vao.enableAttrib(1);
    // texCoord: location 2
    _vao.setAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    _vao.enableAttrib(2);

    _vao.unbind();
}

void Mesh::draw() const {
    _vao.bind();
    glDrawElements(GL_TRIANGLES, (GLsizei)_indices.size(), GL_UNSIGNED_INT, 0);
}
