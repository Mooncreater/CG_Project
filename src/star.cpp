#include "star.h"
#include <cmath>

Star::Star(const glm::vec2& position, float rotation, float radius, float aspect)
    : _position(position), _rotation(rotation), _radius(radius) {
    // TODO: assemble the vertex data of the star
    // write your code here
    // -------------------------------------
    for (int i = 0; i < 3; ++i) {

        float outerAngle = _rotation + i * 2 * M_PI / 5;
        float outerX = _position.x + _radius * cos(outerAngle);
        float outerY = _position.y + _radius * sin(outerAngle) ; 
        _vertices.push_back(glm::vec2(outerX, outerY * aspect));
          
        float innerAngle = _rotation + (i -1.5f) * 2 * M_PI / 5;
        float innerRadius = _radius * 0.382f;  
        float innerX = _position.x + innerRadius * cos(innerAngle);
        float innerY = _position.y + innerRadius * sin(innerAngle);
        _vertices.push_back(glm::vec2(innerX, innerY * aspect));
        outerAngle = _rotation + (i + 2) * 2 * M_PI / 5;
        outerX = _position.x + _radius * cos(outerAngle);
        outerY = _position.y + _radius * sin(outerAngle) ; 
        _vertices.push_back(glm::vec2(outerX , outerY * aspect));

    }
    // -------------------------------------

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(
        GL_ARRAY_BUFFER, sizeof(glm::vec2) * _vertices.size(), _vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

Star::Star(Star&& rhs) noexcept
    : _position(rhs._position), _rotation(rhs._rotation), _radius(rhs._radius), _vao(rhs._vao),
      _vbo(rhs._vbo) {
    rhs._vao = 0;
    rhs._vbo = 0;
}

Star::~Star() {
    if (_vbo) {
        glDeleteVertexArrays(1, &_vbo);
        _vbo = 0;
    }

    if (_vao) {
        glDeleteVertexArrays(1, &_vao);
        _vao = 0;
    }
}

void Star::draw() const {
    glBindVertexArray(_vao);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(_vertices.size()));
}