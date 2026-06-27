#pragma once

#include "types.h"
#include "../collision/gjk.h"
#include <vector>
#include <glm/glm.hpp>

class ProjectileSystem {
public:
    ProjectileSystem();

    // Fire from tower toward target enemy (homing)
    void fire(const glm::vec3& origin, int targetEnemy,
              const TowerDef& towerDef,
              float speed = GameConst::PROJECTILE_SPEED);

    struct HitInfo {
        int projectileIndex;
        int enemyIndex;
        float damage;
        float splashRadius;
        float slowFactor;
        float slowDuration;
        glm::vec3 position;
    };

    // Returns hits detected this frame
    std::vector<HitInfo> update(float deltaTime,
                                const std::vector<Enemy>& enemies);

    const std::vector<Projectile>& projectiles() const { return _projectiles; }
    int activeCount() const;

private:
    std::vector<Projectile> _projectiles;

    ConvexHull makeEnemyHull(const Enemy& e) const;
    ConvexHull makeProjectileHull(const Projectile& p) const;
};
