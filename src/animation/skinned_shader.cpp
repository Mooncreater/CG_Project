#include "skinned_shader.h"
#include <stdexcept>
#include <iostream>

SkinnedShader::SkinnedShader() {
    // MAX_BONES=20 keeps us well under the 1024 uniform component limit
    const char* vs = R"(#version 330 core
        layout(location=0) in vec3 aP;
        layout(location=1) in vec3 aN;
        layout(location=2) in vec2 aT;
        layout(location=3) in ivec4 aBoneIds;
        layout(location=4) in vec4 aWeights;

        uniform mat4 uBones[20];
        uniform mat4 uMVP;  // combined model*view*projection
        uniform mat4 uM;    // model for normal transform + world pos

        out vec3 vWorldPos, vWorldNormal;

        void main() {
            mat4 skin = uBones[aBoneIds.x] * aWeights.x
                      + uBones[aBoneIds.y] * aWeights.y
                      + uBones[aBoneIds.z] * aWeights.z
                      + uBones[aBoneIds.w] * aWeights.w;

            vec4 skinnedPos = skin * vec4(aP, 1.0);
            vec4 skinnedNrm = skin * vec4(aN, 0.0);

            vec4 worldPos = uM * skinnedPos;
            vWorldPos = worldPos.xyz;
            vWorldNormal = normalize(mat3(transpose(inverse(uM))) * skinnedNrm.xyz);
            gl_Position = uMVP * skinnedPos;
        })";

    const char* fs = R"(#version 330 core
        in vec3 vWorldPos, vWorldNormal;
        out vec4 oColor;

        uniform vec3 uColor;
        uniform float uAmbient;
        uniform vec3 uSunDir, uSunColor;
        uniform vec3 uPointPos, uPointColor;
        uniform float uPointIntensity;

        void main() {
            vec3 N = normalize(vWorldNormal);
            vec3 amb = uColor * uAmbient;
            float nd = max(dot(N, normalize(uSunDir)), 0.0);
            vec3 dif = uColor * uSunColor * nd;
            vec3 toPt = uPointPos - vWorldPos;
            float ptDist = length(toPt);
            float ptAtten = uPointIntensity / (ptDist * ptDist + 0.1);
            float ptNd = max(dot(N, normalize(toPt)), 0.0);
            vec3 pt = uColor * uPointColor * ptNd * ptAtten;
            oColor = vec4(amb + dif + pt, 1.0);
        })";

    _prog = std::make_unique<GLSLProgram>();
    _prog->attachVertexShader(vs);
    _prog->attachFragmentShader(fs);
    _prog->link();

    _bonesLoc = glGetUniformLocation(_prog->getHandle(), "uBones");
    if (_bonesLoc < 0)
        std::cerr << "WARNING: uBones uniform not found!" << std::endl;
}

void SkinnedShader::use() { _prog->use(); }

void SkinnedShader::setMatrices(const glm::mat4& proj, const glm::mat4& view, const glm::mat4& model) {
    glm::mat4 mvp = proj * view * model;
    _prog->setUniformMat4("uMVP", mvp);
    _prog->setUniformMat4("uM", model);
}

void SkinnedShader::setBoneMatrices(const std::vector<glm::mat4>& mats) {
    setBoneMatrices(mats.data(), (int)mats.size());
}

void SkinnedShader::setBoneMatrices(const glm::mat4* mats, int count) {
    if (_bonesLoc >= 0 && count > 0 && count <= 20) {
        glUniformMatrix4fv(_bonesLoc, count, GL_FALSE, &mats[0][0][0]);
    }
}

void SkinnedShader::setColor(const glm::vec3& color) {
    _prog->setUniformVec3("uColor", color);
}

void SkinnedShader::setLight(const glm::vec3& sunDir, const glm::vec3& sunColor,
                              const glm::vec3& pointPos, const glm::vec3& pointColor,
                              float pointIntensity) {
    _prog->setUniformVec3("uSunDir", sunDir);
    _prog->setUniformVec3("uSunColor", sunColor);
    _prog->setUniformVec3("uPointPos", pointPos);
    _prog->setUniformVec3("uPointColor", pointColor);
    _prog->setUniformFloat("uPointIntensity", pointIntensity);
    _prog->setUniformFloat("uAmbient", 0.25f);
}
