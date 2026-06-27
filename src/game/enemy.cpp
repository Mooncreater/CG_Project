#include "enemy.h"
#include <algorithm>
#include <cmath>

EnemySystem::EnemySystem() {
    _enemies.reserve(GameConst::MAX_ENEMIES);
}

void EnemySystem::addPath(const std::vector<PathNode>& nodes) {
    _paths.push_back(nodes);
}

void EnemySystem::spawnEnemy(int pathId, const EnemyDef& def) {
    if ((int)_enemies.size() >= GameConst::MAX_ENEMIES) return;
    if (pathId < 0 || pathId >= (int)_paths.size()) return;
    if (_paths[pathId].empty()) return;

    Enemy e;
    e.state = EnemyState::Alive;
    e.def = &def;
    e.position = _paths[pathId][0].position;
    e.health = def.maxHealth;
    e.currentSpeed = def.speed;
    e.slowTimer = 0;
    e.slowAmount = 1.0f;
    e.currentPathIndex = 1;
    e.pathProgress = 0.0f;
    e.deathTimer = 0.0f;
    e.pathId = pathId;
    e.flashTimer = 0;

    _enemies.push_back(e);
    _totalSpawned++;
}

int EnemySystem::update(float deltaTime) {
    int reachedBase = 0;

    for (auto& e : _enemies) {
        if (e.state == EnemyState::Dead) continue;

        if (e.state == EnemyState::Dying) {
            e.deathTimer -= deltaTime;
            if (e.deathTimer <= 0.0f)
                e.state = EnemyState::Dead;
            continue;
        }

        // Update slow
        if (e.slowTimer > 0) {
            e.slowTimer -= deltaTime;
            if (e.slowTimer <= 0)
                e.slowAmount = 1.0f;
            e.currentSpeed = e.def->speed * e.slowAmount;
        } else {
            e.currentSpeed = e.def->speed;
            e.slowAmount = 1.0f;
        }

        // Update flash
        if (e.flashTimer > 0) e.flashTimer -= deltaTime;

        // Move along path
        const auto& path = _paths[e.pathId];
        if (e.currentPathIndex >= (int)path.size()) {
            reachedBase++;
            e.state = EnemyState::Dead;
            continue;
        }

        const PathNode& target = path[e.currentPathIndex];
        glm::vec3 dir = target.position - e.position;
        float dist = glm::length(dir);

        if (dist < 0.25f) {
            e.position = target.position;
            e.currentPathIndex++;
        } else {
            glm::vec3 moveDir = dir / dist;
            e.position += moveDir * e.currentSpeed * deltaTime;
        }
    }

    // Remove dead enemies
    _enemies.erase(
        std::remove_if(_enemies.begin(), _enemies.end(),
            [](const Enemy& e) { return e.state == EnemyState::Dead; }),
        _enemies.end());

    return reachedBase;
}

bool EnemySystem::damageEnemy(int index, float damage) {
    if (index < 0 || index >= (int)_enemies.size()) return false;
    Enemy& e = _enemies[index];
    if (e.state != EnemyState::Alive) return false;

    e.health -= damage;
    e.flashTimer = 0.15f;
    if (e.health <= 0.0f) {
        e.state = EnemyState::Dying;
        e.deathTimer = 0.3f;
        _totalKilled++;
        return true;
    }
    return false;
}

void EnemySystem::applySlow(int index, float factor, float duration) {
    if (index < 0 || index >= (int)_enemies.size()) return;
    Enemy& e = _enemies[index];
    if (e.state != EnemyState::Alive) return;

    // Use stronger slow (lower factor)
    if (factor < e.slowAmount || e.slowTimer <= 0) {
        e.slowAmount = factor;
        e.slowTimer = duration;
    }
}

void EnemySystem::applySplashSlow(const glm::vec3& center, float radius,
                                   float factor, float duration) {
    float r2 = radius * radius;
    for (int i = 0; i < (int)_enemies.size(); i++) {
        if (_enemies[i].state != EnemyState::Alive) continue;
        glm::vec3 d = _enemies[i].position - center;
        d.y = 0; // 2D distance on XZ plane
        if (glm::dot(d, d) <= r2)
            applySlow(i, factor, duration);
    }
}

int EnemySystem::aliveCount() const {
    int cnt = 0;
    for (const auto& e : _enemies)
        if (e.state == EnemyState::Alive) cnt++;
    return cnt;
}
