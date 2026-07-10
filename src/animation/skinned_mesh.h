#pragma once
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include <glad/gl.h>

// Vertex with skinning data: 4 bone indices + 4 weights
struct SkinnedVertex {
    glm::vec3 position{0};
    glm::vec3 normal{0,1,0};
    glm::vec2 texCoord{0};
    int boneIds[4] = {0,0,0,0};
    float weights[4] = {1,0,0,0};

    SkinnedVertex() = default;
};

// GPU mesh that supports skeletal animation
class SkinnedMesh {
public:
    SkinnedMesh() = default;
    SkinnedMesh(const std::vector<SkinnedVertex>& vertices,
                const std::vector<uint32_t>& indices);
    ~SkinnedMesh();

    SkinnedMesh(SkinnedMesh&& o) noexcept;
    SkinnedMesh& operator=(SkinnedMesh&& o) noexcept;
    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;

    void draw() const;
    void drawRange(uint32_t startIndex, uint32_t count) const;
    uint32_t indexCount() const { return _indexCount; }

private:
    GLuint _vao = 0, _vbo = 0, _ebo = 0;
    uint32_t _indexCount = 0;
};
