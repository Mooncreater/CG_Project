#include "skinned_mesh.h"
#include <stdexcept>

SkinnedMesh::SkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                         const std::vector<uint32_t>& indices)
    : _indexCount((uint32_t)indices.size())
{
    if (vertices.empty()) return;

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    if (!indices.empty()) glGenBuffers(1, &_ebo);

    glBindVertexArray(_vao);

    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices.size() * sizeof(SkinnedVertex)),
                 vertices.data(), GL_STATIC_DRAW);

    // EBO
    if (!indices.empty()) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(indices.size() * sizeof(uint32_t)),
                     indices.data(), GL_STATIC_DRAW);
    }

    // Position  loc=0
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (void*)offsetof(SkinnedVertex, position));
    // Normal    loc=1
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (void*)offsetof(SkinnedVertex, normal));
    // TexCoord  loc=2
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (void*)offsetof(SkinnedVertex, texCoord));
    // BoneIds   loc=3  (ivec4)
    glEnableVertexAttribArray(3);
    glVertexAttribIPointer(3, 4, GL_INT, sizeof(SkinnedVertex),
                           (void*)offsetof(SkinnedVertex, boneIds));
    // Weights   loc=4  (vec4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex),
                          (void*)offsetof(SkinnedVertex, weights));

    glBindVertexArray(0);
}

SkinnedMesh::~SkinnedMesh() {
    if (_vao) glDeleteVertexArrays(1, &_vao);
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_ebo) glDeleteBuffers(1, &_ebo);
}

SkinnedMesh::SkinnedMesh(SkinnedMesh&& o) noexcept
    : _vao(o._vao), _vbo(o._vbo), _ebo(o._ebo), _indexCount(o._indexCount) {
    o._vao = o._vbo = o._ebo = 0;
}

SkinnedMesh& SkinnedMesh::operator=(SkinnedMesh&& o) noexcept {
    if (this != &o) {
        if (_vao) glDeleteVertexArrays(1, &_vao);
        if (_vbo) glDeleteBuffers(1, &_vbo);
        if (_ebo) glDeleteBuffers(1, &_ebo);
        _vao = o._vao; _vbo = o._vbo; _ebo = o._ebo; _indexCount = o._indexCount;
        o._vao = o._vbo = o._ebo = 0;
    }
    return *this;
}

void SkinnedMesh::draw() const {
    if (_vao == 0 || _indexCount == 0) return;
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)_indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
