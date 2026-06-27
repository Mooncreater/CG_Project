#include "projectile.h"
#include <algorithm>
#include <cmath>

ProjectileSystem::ProjectileSystem() {
    _projectiles.reserve(GameConst::MAX_PROJECTILES);
}

void ProjectileSystem::fire(const glm::vec3& origin, int targetEnemy,
                             const TowerDef& towerDef, float speed) {
    if ((int)_projectiles.size() >= GameConst::MAX_PROJECTILES) return;

    Projectile p;
    p.position = origin;
    p.velocity = glm::vec3(0, 0, -1); // placeholder, updated each frame
    p.lifetime = GameConst::PROJECTILE_LIFETIME;
    p.damage = towerDef.damage;
    p.radius = 0.12f;
    p.splashRadius = towerDef.splashRadius;
    p.slowFactor = towerDef.slowFactor;
    p.slowDuration = towerDef.slowDuration;
    p.alive = true;
    p.targetEnemyIndex = targetEnemy;
    p.color = towerDef.projectileColor;
    _projectiles.push_back(p);
}

std::vector<ProjectileSystem::HitInfo>
ProjectileSystem::update(float deltaTime,
                          const std::vector<Enemy>& enemies) {
    std::vector<HitInfo> hits;

    for (auto& p : _projectiles) {
        if (!p.alive) continue;

        // Home toward target enemy
        if (p.targetEnemyIndex >= 0 &&
            p.targetEnemyIndex < (int)enemies.size() &&
            enemies[p.targetEnemyIndex].state == EnemyState::Alive) {
            glm::vec3 toTarget = enemies[p.targetEnemyIndex].position
                               - p.position;
            float dist = glm::length(toTarget);
            if (dist > 0.001f) {
                glm::vec3 desired = (toTarget / dist) * GameConst::PROJECTILE_SPEED;
                // Smooth homing
                p.velocity = glm::normalize(
                    glm::mix(glm::normalize(p.velocity), glm::normalize(desired), deltaTime * 8.0f)
                ) * GameConst::PROJECTILE_SPEED;
            }
        }

        p.position += p.velocity * deltaTime;
        p.lifetime -= deltaTime;

        if (p.lifetime <= 0.0f) {
            p.alive = false;
            continue;
        }

        // Check collision against alive enemies using GJK
        ConvexHull projHull = makeProjectileHull(p);
        for (int ei = 0; ei < (int)enemies.size(); ei++) {
            const Enemy& e = enemies[ei];
            if (e.state != EnemyState::Alive) continue;

            ConvexHull enemyHull = makeEnemyHull(e);
            if (GJK::intersect(projHull, enemyHull)) {
                hits.push_back({
                    (int)(&p - &_projectiles[0]),
                    ei,
                    p.damage,
                    p.splashRadius,
                    p.slowFactor,
                    p.slowDuration,
                    p.position
                });
                p.alive = false;
                break;
            }
        }
    }

    // Remove dead projectiles
    _projectiles.erase(
        std::remove_if(_projectiles.begin(), _projectiles.end(),
            [](const Projectile& p) { return !p.alive; }),
        _projectiles.end());

    return hits;
}

int ProjectileSystem::activeCount() const {
    return (int)std::count_if(_projectiles.begin(), _projectiles.end(),
        [](const Projectile& p) { return p.alive; });
}

ConvexHull ProjectileSystem::makeEnemyHull(const Enemy& e) const {
    float r = e.def ? e.def->radius : 0.4f;
    float h = e.def ? e.def->height : 1.0f;
    float yBot = e.position.y - h * 0.5f;
    float yTop = e.position.y + h * 0.5f;

    ConvexHull hull;
    int segs = 8;
    for (int i = 0; i < segs; i++) {
        float angle = 2.0f * 3.14159265f * i / segs;
        float px = e.position.x + cosf(angle) * r;
        float pz = e.position.z + sinf(angle) * r;
        hull.vertices.push_back(glm::vec3(px, yBot, pz));
        hull.vertices.push_back(glm::vec3(px, yTop, pz));
    }
    return hull;
}

ConvexHull ProjectileSystem::makeProjectileHull(const Projectile& p) const {
    ConvexHull hull;
    float r = p.radius;
    hull.vertices.push_back(p.position + glm::vec3( r, 0, 0));
    hull.vertices.push_back(p.position + glm::vec3(-r, 0, 0));
    hull.vertices.push_back(p.position + glm::vec3( 0, r, 0));
    hull.vertices.push_back(p.position + glm::vec3( 0,-r, 0));
    hull.vertices.push_back(p.position + glm::vec3( 0, 0, r));
    hull.vertices.push_back(p.position + glm::vec3( 0, 0,-r));
    return hull;
}
