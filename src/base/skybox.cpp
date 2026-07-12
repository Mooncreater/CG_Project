#include "skybox.h"

SkyBox::SkyBox(const std::vector<std::string>& textureFilenames) {
    GLfloat vertices[] = {-1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f,
                          1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f,

                          -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f,
                          -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,

                          1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,
                          1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f,

                          -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
                          1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,

                          -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,
                          1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f,

                          -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
                          1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f};

    _vao.bind();
    _vbo.bind();
    _vbo.setData(sizeof(vertices), vertices, GL_STATIC_DRAW);
    _vao.enableAttrib(0);
    _vao.setAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
    _vao.unbind();

    try {
        _texture.reset(new ImageTextureCubemap(textureFilenames));

        const char* vsCode =
            "#version 330 core\n"
            "layout(location = 0) in vec3 aPosition;\n"
            "out vec3 texCoord;\n"
            "uniform mat4 projection;\n"
            "uniform mat4 view;\n"
            "void main() {\n"
            "   texCoord = aPosition;\n"
            "   vec4 pos = projection * mat4(mat3(view)) * vec4(aPosition, 1.0);\n"
            "   gl_Position = pos.xyww;\n"
            "}\n";

        const char* fsCode =
            "#version 330 core\n"
            "out vec4 color;\n"
            "in vec3 texCoord;\n"
            "uniform samplerCube cubemap;\n"
            "void main() {\n"
            "   color = texture(cubemap, texCoord);\n"
            "}\n";

        _shader.reset(new GLSLProgram);
        _shader->attachVertexShader(vsCode);
        _shader->attachFragmentShader(fsCode);
        _shader->link();
    } catch (const std::exception&) {
        throw;
    }
}

void SkyBox::draw(const glm::mat4& projection, const glm::mat4& view) {
    glDepthFunc(GL_LEQUAL);
    _shader->use();
    _shader->setUniformMat4("projection", projection);
    // Strip translation: skybox should follow camera rotation only
    glm::mat4 viewNoTrans = glm::mat4(glm::mat3(view));
    _shader->setUniformMat4("view", viewNoTrans);
    _texture->bind(0);
    _vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glDepthFunc(GL_LESS);
}
