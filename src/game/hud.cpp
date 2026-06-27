#include "hud.h"
#include <glad/gl.h>
#include <algorithm>

HUD::HUD() {}
HUD::~HUD() {
    if (_vbo) glDeleteBuffers(1, &_vbo);
    if (_vao) glDeleteVertexArrays(1, &_vao);
}

void HUD::init() {
    const char* vs = R"(#version 330 core
        layout(location=0) in vec2 aPos;
        uniform vec2 uPos, uSize;
        void main() { vec2 p = aPos * uSize + uPos; gl_Position = vec4(p, 0.0, 1.0); })";
    const char* fs = R"(#version 330 core
        out vec4 oc; uniform vec4 uColor;
        void main() { oc = uColor; })";
    _shader = std::make_unique<GLSLProgram>();
    _shader->attachVertexShader(vs);
    _shader->attachFragmentShader(fs);
    _shader->link();

    float quad[] = { -1,-1, 1,-1, 1,1, -1,-1, 1,1, -1,1 };
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glBindVertexArray(0);
}

void HUD::renderIngame(float carrotHp, int gold, int wave, int totalWaves,
                        int score, int kills, int selectedTower) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _shader->use();

    // === Top bar background ===
    drawBox(-1.0f, 0.86f, 2.0f, 0.14f, glm::vec4(0.05f, 0.05f, 0.12f, 0.85f));

    // === Carrot HP bar (left) ===
    float hp = carrotHp / GameConst::CARROT_MAX_HP;
    glm::vec3 hc = hp > 0.6f ? glm::vec3(0.15f, 0.85f, 0.2f)
                 : hp > 0.3f ? glm::vec3(0.9f, 0.75f, 0.1f)
                 : glm::vec3(0.9f, 0.15f, 0.1f);
    drawBar(-0.92f, 0.885f, 0.3f, 0.025f, hp, hc, glm::vec3(0.12f));

    // === Gold display (center) ===
    drawBox(-0.08f, 0.89f, 0.16f, 0.03f, glm::vec4(0.8f, 0.7f, 0.1f, 0.8f));

    // === Wave display (right) ===
    drawBox(0.55f, 0.89f, 0.12f, 0.03f, glm::vec4(0.3f, 0.5f, 0.8f, 0.8f));

    // === Bottom tower panel ===
    drawBox(-1.0f, -1.0f, 2.0f, 0.18f, glm::vec4(0.05f, 0.05f, 0.12f, 0.85f));

    // Tower selection buttons
    float bx = -0.85f, bw = 0.14f, bh = 0.1f, gap = 0.02f;
    for (int i = 0; i < NUM_TOWER_TYPES; i++) {
        float alpha = (i == selectedTower) ? 1.0f : 0.6f;
        float border = (i == selectedTower) ? 0.003f : 0.001f;
        // Border
        if (selectedTower == i) {
            drawBox(bx - border, -0.96f - border,
                    bw + border * 2, bh + border * 2,
                    glm::vec4(1, 1, 0, 0.9f));
        }
        // Button
        glm::vec3 tc = TOWER_DEFS[i].color;
        drawBox(bx, -0.96f, bw, bh, glm::vec4(tc, alpha));
        // Cost indicator
        drawBox(bx, -1.0f + bh * 0.05f, bw, 0.015f,
                glm::vec4(0.8f, 0.7f, 0.1f, 0.9f));
        bx += bw + gap;
    }

    // Sell button
    drawBox(0.78f, -0.96f, 0.1f, bh, glm::vec4(0.7f, 0.15f, 0.1f, 0.7f));

    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

void HUD::renderEndScreen(bool victory, int score, int kills, int wave) {
    glDisable(GL_DEPTH_TEST); glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _shader->use();

    float a = victory ? 0.4f : 0.55f;
    drawOverlay(glm::vec4(0, 0, 0, 1), a);

    float bw = 0.5f, bh = 0.07f, bx = -bw * 0.5f;
    glm::vec4 bc = victory ? glm::vec4(0.1f, 0.5f, 0.2f, 0.9f)
                           : glm::vec4(0.6f, 0.15f, 0.1f, 0.9f);
    drawBox(bx - 0.01f, 0.05f, bw + 0.02f, bh + 0.02f,
            glm::vec4(bc.r * 0.4f, bc.g * 0.4f, bc.b * 0.4f, 0.5f));
    drawBox(bx, 0.05f, bw, bh, bc);

    glDisable(GL_BLEND); glEnable(GL_DEPTH_TEST);
}

// ============================================================
void HUD::drawBox(float sx, float sy, float w, float h, const glm::vec4& c) {
    _shader->setUniformVec2("uPos", glm::vec2(sx, sy));
    _shader->setUniformVec2("uSize", glm::vec2(w, h));
    _shader->setUniformVec4("uColor", c);
    glBindVertexArray(_vao); glDrawArrays(GL_TRIANGLES, 0, 6);
}

void HUD::drawBar(float sx, float sy, float w, float h, float fill,
                   const glm::vec3& c, const glm::vec3& bg) {
    _shader->setUniformVec2("uPos", glm::vec2(sx, sy));
    _shader->setUniformVec2("uSize", glm::vec2(w, h));
    _shader->setUniformVec4("uColor", glm::vec4(bg, 0.8f));
    glBindVertexArray(_vao); glDrawArrays(GL_TRIANGLES, 0, 6);
    float fw = w * std::max(0.0f, std::min(1.0f, fill));
    if (fw > 0.001f) {
        _shader->setUniformVec2("uPos", glm::vec2(sx + fw - w, sy));
        _shader->setUniformVec2("uSize", glm::vec2(fw, h));
        _shader->setUniformVec4("uColor", glm::vec4(c, 0.9f));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

void HUD::drawOverlay(const glm::vec4& c, float a) {
    _shader->setUniformVec2("uPos", glm::vec2(-1, -1));
    _shader->setUniformVec2("uSize", glm::vec2(2, 2));
    _shader->setUniformVec4("uColor", glm::vec4(c.r, c.g, c.b, a));
    glBindVertexArray(_vao); glDrawArrays(GL_TRIANGLES, 0, 6);
}
