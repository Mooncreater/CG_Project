#pragma once

#include <memory>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "buffer.h"
#include "gl_utility.h"
#include "glsl_program.h"
#include "texture_cubemap.h"
#include "vertex_array.h"

class SkyBox {
public:
    SkyBox(const std::vector<std::string>& textureFilenames);

    SkyBox(SkyBox&& rhs) noexcept = default;

    ~SkyBox() = default;

    void draw(const glm::mat4& projection, const glm::mat4& view);

private:
    VertexArray _vao;
    VertexBuffer _vbo;

    std::unique_ptr<TextureCubemap> _texture;

    std::unique_ptr<GLSLProgram> _shader;
};
