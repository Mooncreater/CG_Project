#include "tower.h"
#include <algorithm>
#include <cmath>

TowerSystem::TowerSystem() {
    _towers.reserve(GameConst::MAX_TOWERS);
}

bool TowerSystem::canPlace(int gridX, int gridY, int cellType) const {
    // Must be buildable/empty and not already occupied
    if (cellType != GameConst::Buildable && cellType != GameConst::Empty)
        return false;
    return towerAt(gridX, gridY) < 0;
}

bool TowerSystem::placeTower(int gridX, int gridY, TowerType type,
                              const glm::vec3& worldPos) {
    if ((int)_towers.size() >= GameConst::MAX_TOWERS) return false;

    const TowerDef* def = nullptr;
    for (int i = 0; i < NUM_TOWER_TYPES; i++) {
        if (TOWER_DEFS[i].type == type) { def = &TOWER_DEFS[i]; break; }
    }
    if (!def) return false;

    Tower t;
    t.type = type;
    t.def = def;
    t.position = worldPos + glm::vec3(0, 0.6f, 0); // slightly above ground
    t.gridX = gridX;
    t.gridY = gridY;
    t.cooldown = 0;
    t.targetIndex = -1;
    t.rotation = 0;
    _towers.push_back(t);
    return true;
}

void TowerSystem::sellTower(int index) {
    if (index < 0 || index >= (int)_towers.size()) return;
    _towers.erase(_towers.begin() + index);
}

int TowerSystem::towerAt(int gridX, int gridY) const {
    for (int i = 0; i < (int)_towers.size(); i++) {
        if (_towers[i].gridX == gridX && _towers[i].gridY == gridY)
            return i;
    }
    return -1;
}

const Tower* TowerSystem::getTower(int index) const {
    if (index < 0 || index >= (int)_towers.size()) return nullptr;
    return &_towers[index];
}

int TowerSystem::sellValue(int index) const {
    if (index < 0 || index >= (int)_towers.size()) return 0;
    return (int)(_towers[index].def->cost * GameConst::SELL_REFUND_RATIO);
}

int TowerSystem::findTarget(const Tower& tower,
                             const std::vector<Enemy>& enemies) const {
    int bestIdx = -1;
    float bestProgress = -1;
    float range2 = tower.def->range * tower.def->range;

    for (int i = 0; i < (int)enemies.size(); i++) {
        if (enemies[i].state != EnemyState::Alive) continue;
        glm::vec3 d = enemies[i].position - tower.position;
        d.y = 0;
        float dist2 = glm::dot(d, d);
        if (dist2 > range2) continue;

        // Target enemy farthest along path (highest path index + progress)
        float progress = enemies[i].currentPathIndex + enemies[i].pathProgress;
        if (progress > bestProgress) {
            bestProgress = progress;
            bestIdx = i;
        }
    }
    return bestIdx;
}

void TowerSystem::update(float deltaTime,
                          const std::vector<Enemy>& enemies) {
    for (auto& t : _towers) {
        t.cooldown -= deltaTime;
        if (t.cooldown < 0) t.cooldown = 0;
        t.targetIndex = findTarget(t, enemies);

        // Face target
        if (t.targetIndex >= 0 && t.targetIndex < (int)enemies.size()) {
            glm::vec3 d = enemies[t.targetIndex].position - t.position;
            t.rotation = atan2f(d.x, d.z);
        }
    }
}

std::vector<TowerSystem::FireCommand> TowerSystem::getFireCommands() {
    std::vector<FireCommand> commands;
    for (int i = 0; i < (int)_towers.size(); i++) {
        Tower& t = _towers[i];
        if (t.cooldown > 0) continue;
        if (t.targetIndex < 0) continue;

        FireCommand cmd;
        cmd.towerIndex = i;
        cmd.targetEnemyIndex = t.targetIndex;
        cmd.def = t.def;
        cmd.position = t.position;
        t.cooldown = 1.0f / t.def->fireRate;
        commands.push_back(cmd);
    }
    return commands;
}
