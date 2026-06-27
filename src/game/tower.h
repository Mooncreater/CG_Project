#pragma once

#include "types.h"
#include <vector>
#include <glm/glm.hpp>

class TowerSystem {
public:
    TowerSystem();

    bool canPlace(int gridX, int gridY, int cellType) const;
    bool placeTower(int gridX, int gridY, TowerType type,
                    const glm::vec3& worldPos);
    void sellTower(int index);

    // Update: decrement cooldowns, find targets
    void update(float deltaTime,
                const std::vector<Enemy>& enemies);

    // Towers ready to fire (index, target position, target enemy index)
    struct FireCommand {
        int towerIndex;
        glm::vec3 targetPos;
        int targetEnemyIndex;
        const TowerDef* def;
        glm::vec3 position;
    };
    std::vector<FireCommand> getFireCommands();

    const std::vector<Tower>& towers() const { return _towers; }
    int towerCount() const { return (int)_towers.size(); }

    // Tower at grid cell
    int towerAt(int gridX, int gridY) const;
    const Tower* getTower(int index) const;

    int sellValue(int index) const;

private:
    std::vector<Tower> _towers;

    // Find best target for a tower (closest to carrot / most progress)
    int findTarget(const Tower& tower,
                   const std::vector<Enemy>& enemies) const;
};
