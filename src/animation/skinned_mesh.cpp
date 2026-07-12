#include "skinned_mesh.h"
#include <stdexcept>

SkinnedMesh::SkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                         const std::vector<uint32_t>& indices)
    : _indexCount((uint32_t)indices.size())
{
    if (vertices.empty()) return;

    _vao.bind();
    _vbo.bind();
    _vbo.setData(
        static_cast<GLsizeiptr>(vertices.size() * sizeof(SkinnedVertex)),
        vertices.data(),
        GL_STATIC_DRAW);

    if (!indices.empty()) {
        _ebo.bind();
        _ebo.setData(
            static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
            indices.data(),
            GL_STATIC_DRAW);
    }

    // Position  loc=0
    _vao.enableAttrib(0);
    _vao.setAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, position));
    // Normal    loc=1
    _vao.enableAttrib(1);
    _vao.setAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, normal));
    // TexCoord  loc=2
    _vao.enableAttrib(2);
    _vao.setAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, texCoord));
    // BoneIds   loc=3  (ivec4)
    _vao.enableAttrib(3);
    _vao.setAttribIPointer(3, 4, GL_INT, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, boneIds));
    // Weights   loc=4  (vec4)
    _vao.enableAttrib(4);
    _vao.setAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(SkinnedVertex), (void*)offsetof(SkinnedVertex, weights));

    _vao.unbind();
}

void SkinnedMesh::draw() const {
    if (_vao.getHandle() == 0 || _indexCount == 0) return;
    _vao.bind();
    glDrawElements(GL_TRIANGLES, (GLsizei)_indexCount, GL_UNSIGNED_INT, 0);
}

void SkinnedMesh::drawRange(uint32_t startIndex, uint32_t count) const {
    if (_vao.getHandle() == 0 || count == 0) return;
    _vao.bind();
    glDrawElements(GL_TRIANGLES, (GLsizei)count, GL_UNSIGNED_INT,
                   (void*)(startIndex * sizeof(uint32_t)));
}
