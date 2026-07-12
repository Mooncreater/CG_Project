#include "fullscreen_quad.h"

FullscreenQuad::FullscreenQuad() {
    float _vertices[] = {-1.0f, 1.0f,  0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f,
                         1.0f,  -1.0f, 1.0f, 0.0f, -1.0f, 1.0f,  0.0f, 1.0f,
                         1.0f,  -1.0f, 1.0f, 0.0f, 1.0f,  1.0f,  1.0f, 1.0f};

    _vao.bind();
    _vbo.bind();
    _vbo.setData(sizeof(_vertices), _vertices, GL_STATIC_DRAW);

    _vao.enableAttrib(0);
    _vao.setAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    _vao.enableAttrib(1);
    _vao.setAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void*>(2 * sizeof(float)));

    _vbo.unbind();
    _vao.unbind();
}

void FullscreenQuad::draw() const {
    _vao.bind();
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
