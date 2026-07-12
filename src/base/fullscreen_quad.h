#pragma once

#include <glm/glm.hpp>

#include "buffer.h"
#include "gl_utility.h"
#include "texture.h"
#include "vertex_array.h"

class FullscreenQuad {
public:
    FullscreenQuad();

    FullscreenQuad(const FullscreenQuad&) = delete;

    FullscreenQuad(FullscreenQuad&& rhs) noexcept = default;

    ~FullscreenQuad() = default;

    void draw() const;

private:
    VertexArray _vao;
    VertexBuffer _vbo;
};
