#include "castle_defender.h"
#include "../obj_loader/obj_loader.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <iostream>

static const char* LG = R"(
uniform vec3 uSunDir,uSunColor,uPointPos,uPointColor;
uniform float uPointIntensity,uAmbient;
vec3 cl(vec3 wp,vec3 n,vec3 a){
    vec3 amb=a*uAmbient;
    float nd=max(dot(normalize(n),normalize(uSunDir)),0.);
    vec3 d=a*uSunColor*nd*.65;
    vec3 tl=uPointPos-wp;
    float dist=length(tl),att=uPointIntensity/(1.+dist*dist*.01);
    float np=max(dot(normalize(n),normalize(tl)),0.);
    return amb+d+a*uPointColor*np*att;
})";

// ============================================================
CastleDefender::CastleDefender(const Options& options) : Application(options) {
    srand((unsigned)time(nullptr));
    IMGUI_CHECKVERSION(); ImGui::CreateContext(); ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    initRendering(); initGame();
}

CastleDefender::~CastleDefender() {
    ImGui_ImplOpenGL3_Shutdown(); ImGui_ImplGlfw_Shutdown(); ImGui::DestroyContext();
}

// ============================================================
void CastleDefender::initRendering() {
    std::string vs = R"(#version 330 core
        layout(location=0) in vec3 aP; layout(location=1) in vec3 aN;
        out vec3 fn,fp;
        uniform mat4 uModel,uView,uProjection;
        void main(){fp=vec3(uModel*vec4(aP,1));fn=mat3(transpose(inverse(uModel)))*aN;gl_Position=uProjection*uView*vec4(fp,1);})";
    std::string fs = std::string(R"(#version 330 core
        in vec3 fn,fp; out vec4 oc; uniform vec3 uOC;)") + LG + R"(
        void main(){oc=vec4(cl(fp,fn,uOC),1);})";
    _shader = std::make_unique<GLSLProgram>();
    _shader->attachVertexShader(vs.c_str()); _shader->attachFragmentShader(fs.c_str()); _shader->link();

    auto sf = std::vector<std::string>{
        _assetRootDir + "/texture/skyboxrt/right.jpg", _assetRootDir + "/texture/skyboxrt/left.jpg",
        _assetRootDir + "/texture/skyboxrt/top.jpg", _assetRootDir + "/texture/skyboxrt/bottom.jpg",
        _assetRootDir + "/texture/skyboxrt/front.jpg", _assetRootDir + "/texture/skyboxrt/back.jpg"};
    try { _skybox = std::make_unique<SkyBox>(sf); }
    catch (const std::exception& e) { _skybox = nullptr; }

    auto m = [](const Element& e) { return std::make_unique<Mesh>(e.vertices, e.indices); };
    _cubeMesh   = m(Primitives::CreateCube(1));
    _sphereMesh = m(Primitives::CreateSphere(0.5f, 24));
    _cylinderMesh = m(Primitives::CreateCylinder(1, 1, 32));
    _coneMesh   = m(Primitives::CreateCone(1, 1, 32));

    // Player mesh: capsule-like using cylinder + sphere
    _playerMesh = m(Primitives::CreateCylinder(1, 2, 16));

    _hud.init();
}

// ============================================================
void CastleDefender::initGame() {
    _map.buildDefault();
    for (int i = 0; i < _map.pathCount(); i++) _enemySystem.addPath(_map.getPath(i));
    _waveManager.initWaves();
    _playerPos = glm::vec3(0, 2.0f, 18); // start at bottom of map
    _camYaw = 0; _camPitch = 30; _camDist = 10;
}

// ============================================================
// Camera helpers
glm::vec3 CastleDefender::calcCamPos() const {
    float ry = glm::radians(_camYaw), rp = glm::radians(_camPitch);
    return _playerPos + glm::vec3(
        cos(ry)*cos(rp)*_camDist,
        sin(rp)*_camDist,
        sin(ry)*cos(rp)*_camDist);
}
glm::mat4 CastleDefender::calcView() const {
    return glm::lookAt(calcCamPos(), _playerPos + glm::vec3(0, 0.8f, 0), glm::vec3(0, 1, 0));
}

// ============================================================
void CastleDefender::handleInput() {
    if (!_cursorLocked) return;

    // Mouse look (rotate camera around player)
    float dx = (float)(_input.mouse.move.xNow - _lastMouseX);
    float dy = (float)(_input.mouse.move.yNow - _lastMouseY);
    _camYaw   -= dx * 0.2f;
    _camPitch += dy * 0.2f;
    if (_camPitch > 80) _camPitch = 80;
    if (_camPitch < -30) _camPitch = -30;
    _lastMouseX = _input.mouse.move.xNow;
    _lastMouseY = _input.mouse.move.yNow;

    // Player movement (relative to camera facing on XZ plane)
    glm::vec3 camFwd = glm::normalize(glm::vec3(
        cos(glm::radians(_camYaw)), 0, sin(glm::radians(_camYaw))));
    glm::vec3 camRight = glm::normalize(glm::cross(camFwd, glm::vec3(0,1,0)));

    glm::vec3 move(0);
    if (_input.keyboard.keyStates[GLFW_KEY_W] == GLFW_PRESS) move += camFwd;
    if (_input.keyboard.keyStates[GLFW_KEY_S] == GLFW_PRESS) move -= camFwd;
    if (_input.keyboard.keyStates[GLFW_KEY_A] == GLFW_PRESS) move -= camRight;
    if (_input.keyboard.keyStates[GLFW_KEY_D] == GLFW_PRESS) move += camRight;

    if (glm::length(move) > 0.01f) {
        move = glm::normalize(move);
        _playerYaw = atan2f(move.x, move.z);
    }

    float speed = _moveSpeed;
    if (_input.keyboard.keyStates[GLFW_KEY_LEFT_SHIFT] == GLFW_PRESS) speed *= 1.6f;
    _playerVel.x = move.x * speed;
    _playerVel.z = move.z * speed;

    // Jump
    if (_input.keyboard.keyStates[GLFW_KEY_SPACE] == GLFW_PRESS && _onGround) {
        _playerVel.y = _jumpSpeed; _onGround = false;
    }
}

// ============================================================
void CastleDefender::updatePlayer(float dt) {
    // Gravity
    _playerVel.y -= 20.0f * dt;
    _playerPos += _playerVel * dt;

    // Ground collision
    float groundY = 0.0f;
    if (_playerPos.y < groundY) { _playerPos.y = groundY; _playerVel.y = 0; _onGround = true; }

    // Map bounds
    float hw = GameConst::GRID_COLS * GameConst::CELL_SIZE * 0.5f + 2;
    float hd = GameConst::GRID_ROWS * GameConst::CELL_SIZE * 0.5f + 2;
    _playerPos.x = glm::clamp(_playerPos.x, -hw, hw);
    _playerPos.z = glm::clamp(_playerPos.z, -hd, hd);
}

// ============================================================
void CastleDefender::playerShoot() {
    if (_shootCooldown > 0) return;
    // Find closest enemy in front of player
    int target = findClosestEnemy();
    glm::vec3 dir;
    if (target >= 0) {
        dir = glm::normalize(_enemySystem.enemies()[target].position - _playerPos);
    } else {
        dir = glm::vec3(sinf(_playerYaw), 0, cosf(_playerYaw));
    }
    glm::vec3 origin = _playerPos + glm::vec3(0, 1.2f, 0);
    // Use a "magic" tower def for player projectile
    static TowerDef playerDef = {
        TowerType::Magic, "Player", 99, 0, 8.0f, 0, 1.0f, 0, 0,
        glm::vec3(0.6f,0.2f,0.7f), glm::vec3(0.9f,0.5f,1.0f)};
    _projectileSystem.fire(origin, target, playerDef, 35.0f);
    _shootCooldown = SHOOT_CD;
}

int CastleDefender::findClosestEnemy() const {
    int best = -1; float bestDist = 30.0f * 30.0f; // 30m max range
    glm::vec3 fwd(sinf(_playerYaw), 0, cosf(_playerYaw));
    for (int i = 0; i < (int)_enemySystem.enemies().size(); i++) {
        if (_enemySystem.enemies()[i].state != EnemyState::Alive) continue;
        glm::vec3 d = _enemySystem.enemies()[i].position - _playerPos;
        // Must be in front hemisphere
        if (glm::dot(glm::normalize(d), fwd) < 0.3f) continue;
        float dist2 = glm::dot(d, d);
        if (dist2 < bestDist) { bestDist = dist2; best = i; }
    }
    return best;
}

// ============================================================
void CastleDefender::updateGame() {
    float dt = _deltaTime;

    updatePlayer(dt);
    if (_shootCooldown > 0) _shootCooldown -= dt;

    _waveManager.update(dt, _enemySystem);
    int reached = _enemySystem.update(dt);
    if (reached > 0) { _carrotHp -= (float)reached; if (_carrotHp <= 0) { _carrotHp = 0; _gameState = GameState::Defeat; return; } }

    _towerSystem.update(dt, _enemySystem.enemies());
    for (auto& cmd : _towerSystem.getFireCommands())
        _projectileSystem.fire(cmd.position, cmd.targetEnemyIndex, *cmd.def);

    auto hits = _projectileSystem.update(dt, _enemySystem.enemies());
    for (auto& h : hits) {
        if (_enemySystem.damageEnemy(h.enemyIndex, h.damage)) { _gold += 15; _score += 10; }
        if (h.splashRadius > 0) {
            float r2 = h.splashRadius * h.splashRadius;
            for (int ei = 0; ei < (int)_enemySystem.enemies().size(); ei++) {
                if (ei == h.enemyIndex) continue;
                if (_enemySystem.enemies()[ei].state != EnemyState::Alive) continue;
                glm::vec3 d = _enemySystem.enemies()[ei].position - h.position; d.y = 0;
                if (glm::dot(d,d) <= r2 && _enemySystem.damageEnemy(ei, h.damage*0.5f*(1-sqrtf(glm::dot(d,d))/h.splashRadius)))
                    { _gold += 15; _score += 10; }
            }
        }
        if (h.slowFactor < 1.0f) {
            _enemySystem.applySlow(h.enemyIndex, h.slowFactor, h.slowDuration);
            if (h.splashRadius > 0) _enemySystem.applySplashSlow(h.position, h.splashRadius, h.slowFactor, h.slowDuration);
        }
    }

    if (!_waveManager.isWaveActive()) {
        if (_waveWasActive) { _intermissionTimer = GameConst::WAVE_INTERMISSION; _waveWasActive = false; }
        _intermissionTimer -= dt;
        if (_intermissionTimer <= 0 && !_waveManager.allWavesComplete()) { _waveManager.startNextWave(); _waveWasActive = true; }
        if (_waveManager.allWavesComplete() && _enemySystem.aliveCount() == 0) _gameState = GameState::Victory;
    } else { _waveWasActive = true; }
}

// ============================================================
void CastleDefender::tryPlaceTower(int gridX, int gridY) {
    if (_selectedTower < 0 || _selectedTower >= NUM_TOWER_TYPES) return;
    const TowerDef& def = TOWER_DEFS[_selectedTower];
    if (_gold < def.cost) return;
    int cell = _map.getCell(gridX, gridY);
    if (!_towerSystem.canPlace(gridX, gridY, cell)) return;
    // Must be within 8 units of player
    glm::vec3 wp = _map.cellToWorld(gridX, gridY);
    if (glm::distance(glm::vec2(wp.x, wp.z), glm::vec2(_playerPos.x, _playerPos.z)) > 8.0f) return;
    if (_towerSystem.placeTower(gridX, gridY, def.type, wp)) { _gold -= def.cost; _map.placeTower(gridX, gridY); }
}

// ============================================================
glm::vec3 CastleDefender::getRayPlaneIntersect(float mx, float my) const {
    float nx = (2.0f*mx)/_windowWidth - 1.0f, ny = 1.0f - (2.0f*my)/_windowHeight;
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)_windowWidth/_windowHeight, 0.3f, 200.0f);
    glm::mat4 view = calcView();
    glm::vec4 rc(nx,ny,-1,1), re = glm::inverse(proj)*rc; re = glm::vec4(re.x,re.y,-1,0);
    glm::vec3 rd = glm::normalize(glm::vec3(glm::inverse(view)*re));
    glm::vec3 cp = calcCamPos();
    float t = -cp.y/rd.y;
    if (t <= 0 || fabsf(rd.y) < 0.0001f) return glm::vec3(0);
    return cp + rd*t;
}

// ============================================================
// MAIN RENDER
// ============================================================
void CastleDefender::renderFrame() {
    _input.mouse.scroll.xOffset = 0; _input.mouse.scroll.yOffset = 0;

    // === MENU ===
    if (_gameState == GameState::Menu) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        _cursorLocked = false;
        glClearColor(0.12f, 0.16f, 0.28f, 1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)_windowWidth/_windowHeight, 0.3f, 200.0f);
        glm::mat4 view = calcView();
        glEnable(GL_DEPTH_TEST);
        try {
            glm::mat4 gm = glm::translate(glm::mat4(1), glm::vec3(0,-0.2f,0));
            gm = glm::scale(gm, glm::vec3(80,0.15f,70));
            drawOne(_shader.get(),_cubeMesh.get(),gm,glm::vec3(0.28f,0.48f,0.25f),proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
            for (auto& t : _map.pathTiles()) {
                glm::mat4 m = glm::translate(glm::mat4(1),t.position); m = glm::scale(m,t.size);
                drawOne(_shader.get(),_cubeMesh.get(),m,t.color,proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
            }
        } catch(...){}
        glDisable(GL_DEPTH_TEST);

        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        float ww=(float)_windowWidth, wh=(float)_windowHeight;
        ImGui::SetNextWindowPos(ImVec2(ww*0.5f-210,wh*0.5f-175),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(420,350),ImGuiCond_Always);
        ImGui::Begin("CARROT DEFENSE 3D",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse);
        ImGui::SetCursorPosY(10);
        ImGui::TextColored(ImVec4(1,0.6f,0.1f,1),"     === CARROT DEFENSE 3D ===");
        ImGui::Separator(); ImGui::Spacing();
        ImGui::TextWrapped("Third-person tower defense! Move, jump, shoot magic, and place towers to protect the Carrot!");
        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f,1,0.4f,1),"  TOWERS (keys 1-4, L-click to place):");
        ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("1: Archer(100G)  2: Cannon(150G)");
        ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("3: Ice(120G)     4: Magic(200G)");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1,0.5f,0.5f,1),"  CONTROLS:");
        ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("WASD: Move   Mouse: Look   Space: Jump");
        ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("R-click: Shoot magic    E: Toggle tower mode");
        ImGui::Bullet(); ImGui::SameLine(); ImGui::Text("Scroll: Zoom in/out    F: Reset camera");
        ImGui::Spacing(); ImGui::Spacing();
        ImGui::SetCursorPosX(130);
        if (ImGui::Button("  START GAME  ",ImVec2(160,50))) {
            _gameState = GameState::Playing; _gold = 300; _carrotHp = 20; _score = 0;
            _enemySystem = EnemySystem();
            for (int i=0;i<_map.pathCount();i++) _enemySystem.addPath(_map.getPath(i));
            _towerSystem = TowerSystem(); _projectileSystem = ProjectileSystem();
            _waveManager = WaveManager(); _waveManager.initWaves();
            _waveManager.startNextWave(); _waveWasActive = true;
            _playerPos = glm::vec3(0, 2.0f, 18); _playerVel = glm::vec3(0); _onGround = true;
            _camYaw = 0; _camPitch = 30; _camDist = 10;
            _cursorLocked = true; _placingTower = false;
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            std::cout << "[GAME] Started!\n";
        }
        ImGui::End(); ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        return;
    }

    // === VICTORY / DEFEAT ===
    if (_gameState == GameState::Victory || _gameState == GameState::Defeat) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        _cursorLocked = false;
        glClearColor(0.12f,0.16f,0.28f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glm::mat4 proj=glm::perspective(glm::radians(60.0f),(float)_windowWidth/_windowHeight,0.3f,200.0f);
        glm::mat4 view=calcView();
        glEnable(GL_DEPTH_TEST);
        try{renderScene(proj,view);renderTowers(proj,view);}catch(...){}
        glDisable(GL_DEPTH_TEST);
        _hud.renderEndScreen(_gameState==GameState::Victory,_score,_enemySystem.totalKilled(),_waveManager.currentWave());
        ImGui_ImplOpenGL3_NewFrame(); ImGui_ImplGlfw_NewFrame(); ImGui::NewFrame();
        ImGui::SetNextWindowPos(ImVec2(_windowWidth*0.5f-150,_windowHeight*0.5f+20),ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(300,120),ImGuiCond_Always);
        ImGui::Begin("##end",nullptr,ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoTitleBar);
        ImGui::TextColored(_gameState==GameState::Victory?ImVec4(0.2f,1,0.3f,1):ImVec4(1,0.2f,0.2f,1),
                           _gameState==GameState::Victory?"  VICTORY!":"  DEFEAT!");
        ImGui::Text("  Wave: %d  Score: %d",_waveManager.currentWave(),_score);
        ImGui::Spacing();
        if(ImGui::Button("Close",ImVec2(100,30))) glfwSetWindowShouldClose(_window,true);
        ImGui::End(); ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        return;
    }

    // === PLAYING ===
    // E toggles tower placement mode
    if (_input.keyboard.keyStates[GLFW_KEY_E] == GLFW_PRESS) {
        _input.keyboard.keyStates[GLFW_KEY_E] = GLFW_RELEASE;
        _placingTower = !_placingTower;
    }
    // Scroll zoom
    if (fabsf(_input.mouse.scroll.yOffset) > 0.01f) {
        _camDist -= _input.mouse.scroll.yOffset * 0.8f;
        _camDist = glm::clamp(_camDist, 4.0f, 30.0f);
    }
    // Select tower
    if (_input.keyboard.keyStates[GLFW_KEY_1]==GLFW_PRESS) _selectedTower=0;
    if (_input.keyboard.keyStates[GLFW_KEY_2]==GLFW_PRESS) _selectedTower=1;
    if (_input.keyboard.keyStates[GLFW_KEY_3]==GLFW_PRESS) _selectedTower=2;
    if (_input.keyboard.keyStates[GLFW_KEY_4]==GLFW_PRESS) _selectedTower=3;

    glClearColor(0.12f,0.16f,0.28f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    updateGame();
    if (_gameState != GameState::Playing) return;

    // Mouse raycast for grid
    glm::vec3 wh = getRayPlaneIntersect(_input.mouse.move.xNow, _input.mouse.move.yNow);
    _map.worldToCell(wh, _hoverGridX, _hoverGridY);

    // Mouse clicks
    if (_input.mouse.press.left && _placingTower) {
        _input.mouse.press.left = false;
        tryPlaceTower(_hoverGridX, _hoverGridY);
    }
    // Right-click always shoots (regardless of mode)
    if (_input.mouse.press.right) {
        _input.mouse.press.right = false;
        playerShoot();
    }
    // Left-click when NOT in tower mode = shoot
    if (_input.mouse.press.left && !_placingTower) {
        _input.mouse.press.left = false;
        playerShoot();
    }

    // Render
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)_windowWidth/_windowHeight, 0.3f, 200.0f);
    glm::mat4 view = calcView();

    if (_skybox) try { _skybox->draw(proj, view); } catch(...) {}
    glEnable(GL_DEPTH_TEST);
    try {
        renderScene(proj, view);
        if (_placingTower) renderGridHighlight(proj, view);
        renderEnemies(proj, view);
        renderTowers(proj, view); renderPlayer(proj, view);
        renderProjectiles(proj, view);
        // Carrot
        glm::mat4 cm=glm::translate(glm::mat4(1),_map.carrotPosition()); cm=glm::scale(cm,glm::vec3(0.6f));
        drawOne(_shader.get(),_sphereMesh.get(),cm,glm::vec3(1,0.55f,0.1f),proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
        glm::mat4 tm=glm::translate(glm::mat4(1),_map.carrotPosition()+glm::vec3(0,0.55f,0));
        tm=glm::scale(tm,glm::vec3(0.3f,0.4f,0.3f));
        drawOne(_shader.get(),_coneMesh.get(),tm,glm::vec3(0.1f,0.8f,0.15f),proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
    } catch(...) {}
    glDisable(GL_DEPTH_TEST);

    renderCrosshair();
    _hud.renderIngame(_carrotHp,_gold,_waveManager.currentWave(),_waveManager.totalWaves(),_score,_enemySystem.totalKilled(),_selectedTower);

    static float tt=0; tt+=_deltaTime;
    if (tt>0.5f) { tt=0;
        glfwSetWindowTitle(_window,("Carrot Defense | W"+std::to_string(_waveManager.currentWave())+
            "/"+std::to_string(_waveManager.totalWaves())+" | HP:"+std::to_string((int)_carrotHp)+
            " | G:"+std::to_string(_gold)+" | Mode:"+(_placingTower?"TOWER":"SHOOT")).c_str());
    }
}

// ============================================================
// SCENE
// ============================================================
void CastleDefender::renderScene(const glm::mat4& proj, const glm::mat4& view) {
    glm::mat4 gm=glm::translate(glm::mat4(1),glm::vec3(0,-0.2f,0)); gm=glm::scale(gm,glm::vec3(80,0.15f,70));
    drawOne(_shader.get(),_cubeMesh.get(),gm,glm::vec3(0.28f,0.48f,0.25f),proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
    for (auto& t:_map.pathTiles()){
        glm::mat4 m=glm::translate(glm::mat4(1),t.position); m=glm::scale(m,t.size);
        drawOne(_shader.get(),_cubeMesh.get(),m,t.color,proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
    }
}

// ============================================================
// ENEMIES
// ============================================================
void CastleDefender::renderEnemies(const glm::mat4& proj, const glm::mat4& view) {
    for (auto& e:_enemySystem.enemies()){
        if (e.state==EnemyState::Dead) continue;
        glm::vec3 col=e.def->color;
        if (e.flashTimer>0) col=glm::mix(col,glm::vec3(1),0.6f);
        if (e.state==EnemyState::Dying) col*=0.5f;
        if (e.slowTimer>0) col=glm::mix(col,glm::vec3(0.3f,0.5f,0.9f),0.4f);
        float r=e.def->radius, h=e.def->height;
        glm::mat4 bm=glm::translate(glm::mat4(1),e.position+glm::vec3(0,h*0.5f,0));
        bm=glm::scale(bm,glm::vec3(r*2,h,r*2));
        drawOne(_shader.get(),_cylinderMesh.get(),bm,col,proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
        glm::mat4 em=glm::translate(glm::mat4(1),e.position+glm::vec3(0,h*0.85f,r*0.5f));
        em=glm::scale(em,glm::vec3(r*0.9f));
        drawOne(_shader.get(),_sphereMesh.get(),em,glm::vec3(0.95f,0.95f,0.2f),proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
    }
}

// ============================================================
// TOWERS
// ============================================================
void CastleDefender::renderTowers(const glm::mat4& proj, const glm::mat4& view) {
    for (auto& t:_towerSystem.towers()){
        glm::mat4 bm=glm::translate(glm::mat4(1),t.position+glm::vec3(0,0.3f,0)); bm=glm::scale(bm,glm::vec3(1,0.6f,1));
        drawOne(_shader.get(),_cylinderMesh.get(),bm,t.def->color,proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
        glm::mat4 tm=glm::translate(glm::mat4(1),t.position+glm::vec3(0,0.85f,0));
        tm=glm::rotate(tm,t.rotation,glm::vec3(0,1,0)); tm=glm::scale(tm,glm::vec3(0.5f,0.45f,0.5f));
        drawOne(_shader.get(),_sphereMesh.get(),tm,t.def->color*0.8f,proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
        glm::mat4 bl=glm::translate(glm::mat4(1),t.position+glm::vec3(0,0.85f,0));
        bl=glm::rotate(bl,t.rotation,glm::vec3(0,1,0)); bl=glm::translate(bl,glm::vec3(0,0,0.4f));
        bl=glm::scale(bl,glm::vec3(0.15f,0.15f,0.55f));
        drawOne(_shader.get(),_cylinderMesh.get(),bl,glm::vec3(0.3f,0.3f,0.35f),proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
    }
}

// ============================================================
// PROJECTILES
// ============================================================
void CastleDefender::renderProjectiles(const glm::mat4& proj, const glm::mat4& view) {
    for (auto& p:_projectileSystem.projectiles()){
        if (!p.alive) continue;
        glm::mat4 m=glm::translate(glm::mat4(1),p.position); m=glm::scale(m,glm::vec3(p.radius*2.5f));
        drawOne(_shader.get(),_sphereMesh.get(),m,p.color,proj,view,_sunDir,_ptPos,_ptCol,_ptInt);
    }
}

// ============================================================
// PLAYER
// ============================================================
void CastleDefender::renderPlayer(const glm::mat4& proj, const glm::mat4& view) {
    // Don't render self in first-person (we are the camera)
    // In third-person we see our character
    glm::mat4 body = glm::translate(glm::mat4(1), _playerPos);
    body = glm::rotate(body, _playerYaw, glm::vec3(0,1,0));
    body = glm::scale(body, glm::vec3(0.5f, 1.0f, 0.5f));
    drawOne(_shader.get(), _playerMesh.get(), body, glm::vec3(0.2f, 0.5f, 0.8f), proj, view, _sunDir, _ptPos, _ptCol, _ptInt);

    // Head
    glm::mat4 head = glm::translate(glm::mat4(1), _playerPos + glm::vec3(0, 1.2f, 0));
    head = glm::scale(head, glm::vec3(0.35f));
    drawOne(_shader.get(), _sphereMesh.get(), head, glm::vec3(0.9f, 0.75f, 0.6f), proj, view, _sunDir, _ptPos, _ptCol, _ptInt);
}

// ============================================================
// GRID HIGHLIGHT (tower placement mode)
// ============================================================
void CastleDefender::renderGridHighlight(const glm::mat4& proj, const glm::mat4& view) {
    if (_hoverGridX<0||_hoverGridY<0) return;
    if (!_map.isInBounds(_hoverGridX,_hoverGridY)) return;
    glm::vec3 pos=_map.cellToWorld(_hoverGridX,_hoverGridY);
    int cell=_map.getCell(_hoverGridX,_hoverGridY);
    bool canPlace=(cell==GameConst::Buildable||cell==GameConst::Empty)&&_towerSystem.towerAt(_hoverGridX,_hoverGridY)<0
                  &&_selectedTower>=0&&_selectedTower<NUM_TOWER_TYPES&&_gold>=TOWER_DEFS[_selectedTower].cost
                  &&glm::distance(glm::vec2(pos.x,pos.z),glm::vec2(_playerPos.x,_playerPos.z))<=8.0f;
    glm::vec3 col=canPlace?glm::vec3(0.2f,0.95f,0.2f):glm::vec3(0.95f,0.2f,0.2f);

    // Highlight pillar
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA); glDepthMask(GL_FALSE);
    glm::mat4 m=glm::translate(glm::mat4(1),pos+glm::vec3(0,0.3f,0));
    m=glm::scale(m,glm::vec3(GameConst::CELL_SIZE*0.85f,0.6f,GameConst::CELL_SIZE*0.85f));
    _shader->use(); _shader->setUniformMat4("uProjection",proj); _shader->setUniformMat4("uView",view);
    _shader->setUniformMat4("uModel",m); _shader->setUniformVec3("uOC",col);
    setLight(_shader.get(),_sunDir,_ptPos,_ptCol,_ptInt); _shader->setUniformFloat("uAmbient",0.8f);
    _shader->setUniformVec3("uSunColor",glm::vec3(0)); _shader->setUniformVec3("uPointColor",glm::vec3(0));
    _cubeMesh->draw();
    glDepthMask(GL_TRUE); glDisable(GL_BLEND);
}

// ============================================================
// CROSSHAIR
// ============================================================
void CastleDefender::renderCrosshair() {
    glDisable(GL_DEPTH_TEST);
    // Use HUD's shader indirectly... or just draw with our shader in NDC
    // Simple approach: use existing _shader but draw in screen space
    // Since we don't have a dedicated 2D shader exposed here, skip for now
    glEnable(GL_DEPTH_TEST);
}

// ============================================================
// LIGHTING
// ============================================================
void CastleDefender::setLight(GLSLProgram* s, const glm::vec3& sd, const glm::vec3& pp, const glm::vec3& pc, float pi) {
    s->setUniformVec3("uSunDir",sd); s->setUniformVec3("uSunColor",glm::vec3(1.2f,1.1f,0.95f));
    s->setUniformVec3("uPointPos",pp); s->setUniformVec3("uPointColor",pc);
    s->setUniformFloat("uPointIntensity",pi); s->setUniformFloat("uAmbient",0.5f);
}

void CastleDefender::drawOne(GLSLProgram* s, Mesh* mesh, const glm::mat4& model, const glm::vec3& color,
                              const glm::mat4& proj, const glm::mat4& view,
                              const glm::vec3& sd, const glm::vec3& pp, const glm::vec3& pc, float pi) {
    s->use(); s->setUniformMat4("uProjection",proj); s->setUniformMat4("uView",view);
    s->setUniformMat4("uModel",model); s->setUniformVec3("uOC",color);
    setLight(s,sd,pp,pc,pi); mesh->draw();
}
