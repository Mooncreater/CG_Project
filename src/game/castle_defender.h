#pragma once

#include "../base/application.h"
#include "../base/glsl_program.h"
#include "../base/skybox.h"
#include "../base/texture2d.h"
#include "../camera/free_camera.h"
#include "../mesh/mesh.h"
#include "../primitives/primitives.h"
#include "types.h"
#include "map.h"
#include "enemy.h"
#include "tower.h"
#include "projectile.h"
#include "wave_manager.h"
#include "hud.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class CastleDefender : public Application {
public:
    CastleDefender(const Options& options);
    ~CastleDefender();

private:
    // ===== Rendering =====
    std::unique_ptr<GLSLProgram> _shader;
    std::unique_ptr<SkyBox> _skybox;
    std::unique_ptr<ImageTexture2D> _groundTex;
    std::unique_ptr<Mesh> _cubeMesh, _sphereMesh, _cylinderMesh, _coneMesh;
    std::unique_ptr<Mesh> _playerMesh;

    // ===== Camera =====
    // Third-person orbit: camera orbits around _playerPos
    float _camYaw = 0.0f, _camPitch = 25.0f, _camDist = 12.0f;
    glm::vec3 calcCamPos() const;
    glm::mat4 calcView() const;

    // ===== Player =====
    glm::vec3 _playerPos = glm::vec3(0, 2.0f, 0);
    glm::vec3 _playerVel = glm::vec3(0);
    float _playerYaw = 0.0f;       // facing direction
    bool _onGround = true;
    float _playerRadius = 0.4f;
    float _playerHeight = 1.7f;
    float _moveSpeed = 8.0f;
    float _jumpSpeed = 10.0f;
    float _shootCooldown = 0.0f;
    static constexpr float SHOOT_CD = 0.3f;

    // ===== Game State =====
    GameState _gameState = GameState::Menu;
    GameMap _map;
    EnemySystem _enemySystem;
    TowerSystem _towerSystem;
    ProjectileSystem _projectileSystem;
    WaveManager _waveManager;
    HUD _hud;

    int _gold = 300;
    float _carrotHp = 20.0f;
    int _score = 0;
    int _selectedTower = 0;
    float _intermissionTimer = 0;
    bool _waveWasActive = false;

    // Grid selection
    int _hoverGridX = -1, _hoverGridY = -1;
    bool _placingTower = false;    // toggle: left-click places tower vs shoots

    // Lighting
    glm::vec3 _sunDir = glm::normalize(glm::vec3(0.8f, 1.5f, 1.2f));
    glm::vec3 _ptPos = glm::vec3(10, 12.0f, 5);
    glm::vec3 _ptCol = glm::vec3(1.0f, 0.95f, 0.85f);
    float _ptInt = 8.0f;

    double _lastMouseX = 0, _lastMouseY = 0;
    bool _cursorLocked = false;

    // ===== Methods =====
    void initRendering();
    void initGame();
    void handleInput() override;
    void renderFrame() override;

    // Game logic
    void updateGame();
    void updatePlayer(float dt);
    void playerShoot();
    void tryPlaceTower(int gridX, int gridY);
    int findClosestEnemy() const;

    // Rendering
    void renderScene(const glm::mat4& proj, const glm::mat4& view);
    void renderEnemies(const glm::mat4& proj, const glm::mat4& view);
    void renderTowers(const glm::mat4& proj, const glm::mat4& view);
    void renderProjectiles(const glm::mat4& proj, const glm::mat4& view);
    void renderPlayer(const glm::mat4& proj, const glm::mat4& view);
    void renderGridHighlight(const glm::mat4& proj, const glm::mat4& view);
    void renderCrosshair();

    // Helpers
    glm::vec3 getRayPlaneIntersect(float mx, float my) const;
    static void setLight(GLSLProgram* s, const glm::vec3& sd, const glm::vec3& pp, const glm::vec3& pc, float pi);
    static void drawOne(GLSLProgram* s, Mesh* mesh, const glm::mat4& model, const glm::vec3& color,
                        const glm::mat4& proj, const glm::mat4& view,
                        const glm::vec3& sd, const glm::vec3& pp, const glm::vec3& pc, float pi);
};
