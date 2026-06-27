#pragma once

#include "types.h"
#include "../base/glsl_program.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>

class HUD {
public:
    HUD();
    ~HUD();
    void init();

    void renderIngame(float carrotHp, int gold, int wave, int totalWaves,
                      int score, int kills, int selectedTower);
    void renderEndScreen(bool victory, int score, int kills, int wave);

private:
    std::unique_ptr<GLSLProgram> _shader;
    unsigned int _vao = 0, _vbo = 0;

    void drawBox(float sx, float sy, float w, float h, const glm::vec4& color);
    void drawBar(float sx, float sy, float w, float h, float fill,
                 const glm::vec3& c, const glm::vec3& bg);
    void drawOverlay(const glm::vec4& color, float alpha);
};
