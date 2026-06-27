#pragma once

#include "types.h"
#include <vector>
#include <glm/glm.hpp>

class EnemySystem {
public:
    EnemySystem();

    void addPath(const std::vector<PathNode>& nodes);
    const std::vector<PathNode>& getPath(int id) const { return _paths[id]; }
    int pathCount() const { return (int)_paths.size(); }

    void spawnEnemy(int pathId, const EnemyDef& def);

    // Returns how many enemies reached the carrot this frame
    int update(float deltaTime);

    bool damageEnemy(int index, float damage);
    void applySlow(int index, float factor, float duration);
    void applySplashSlow(const glm::vec3& center, float radius,
                         float factor, float duration);

    const std::vector<Enemy>& enemies() const { return _enemies; }
    std::vector<Enemy>& enemies() { return _enemies; }

    int aliveCount() const;
    int totalSpawned() const { return _totalSpawned; }
    int totalKilled() const { return _totalKilled; }

private:
    std::vector<Enemy> _enemies;
    std::vector<std::vector<PathNode>> _paths;
    int _totalSpawned = 0;
    int _totalKilled = 0;
};
