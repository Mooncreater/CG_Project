#pragma once
#include "../base/glsl_program.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

class SkinnedShader {
public:
    SkinnedShader();
    void use();
    void setMatrices(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
    void setBoneMatrices(const std::vector<glm::mat4>& mats);
    void setBoneMatrices(const glm::mat4* mats, int count);
    void setColor(const glm::vec3& color);
    void setLight(const glm::vec3& sunDir, const glm::vec3& sunColor,
                  const glm::vec3& pointPos, const glm::vec3& pointColor, float pointIntensity);
    void setAmbient(float a) { _prog->setUniformFloat("uAmbient", a); }

private:
    std::unique_ptr<GLSLProgram> _prog;
    GLint _bonesLoc = -1;
};
