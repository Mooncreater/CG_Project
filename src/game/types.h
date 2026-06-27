#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <string>

// ============================================================
//  Tower Defense Game-wide types & constants
// ============================================================

enum class GameState {
    Menu,
    Playing,
    WaveIntermission,
    Victory,
    Defeat
};

enum class EnemyState {
    Alive,
    Dying,
    Dead
};

enum class TowerType {
    Archer,    // 箭塔: fast, single-target
    Cannon,    // 炮塔: slow, splash damage
    Ice,       // 冰塔: slows enemies
    Magic      // 魔法塔: high damage, long range
};

// ---- Tower definition ----
struct TowerDef {
    TowerType type;
    std::string name;
    float range;
    float fireRate;       // shots per second
    float damage;
    float splashRadius;   // 0 for single-target
    float slowFactor;     // 1.0 = no slow, 0.5 = half speed
    float slowDuration;
    int cost;
    glm::vec3 color;
    glm::vec3 projectileColor;
};

// ---- Tower instance ----
struct Tower {
    TowerType type;
    const TowerDef* def;
    glm::vec3 position;
    int gridX, gridY;
    float cooldown;
    int targetIndex;
    float rotation;

    Tower() : type(TowerType::Archer), def(nullptr), position(0),
              gridX(0), gridY(0), cooldown(0), targetIndex(-1), rotation(0) {}
};

// ---- Path node ----
struct PathNode {
    glm::vec3 position;
    float waitTime;
    PathNode(const glm::vec3& pos, float wait = 0.0f)
        : position(pos), waitTime(wait) {}
};

// ---- Enemy definition ----
struct EnemyDef {
    std::string name;
    float speed;
    float maxHealth;
    float damageToBase;
    float radius;
    float height;
    glm::vec3 color;
    int goldValue;
    int scoreValue;
};

// ---- Enemy instance ----
struct Enemy {
    EnemyState state;
    const EnemyDef* def;
    glm::vec3 position;
    float health;
    float currentSpeed;
    float slowTimer;
    float slowAmount;
    int currentPathIndex;
    float pathProgress;
    float deathTimer;
    int pathId;
    float flashTimer;

    Enemy() : state(EnemyState::Alive), def(nullptr), position(0), health(0),
              currentSpeed(0), slowTimer(0), slowAmount(1.0f),
              currentPathIndex(0), pathProgress(0), deathTimer(0),
              pathId(0), flashTimer(0) {}
};

// ---- Projectile ----
struct Projectile {
    glm::vec3 position;
    glm::vec3 velocity;
    float lifetime;
    float damage;
    float radius;
    float splashRadius;
    float slowFactor;
    float slowDuration;
    bool alive;
    int targetEnemyIndex;
    glm::vec3 color;

    Projectile() : position(0), velocity(0), lifetime(0), damage(0),
                   radius(0.12f), splashRadius(0), slowFactor(1.0f),
                   slowDuration(0), alive(false), targetEnemyIndex(-1),
                   color(1,1,0) {}
};

// ---- Wave definition ----
struct WaveDef {
    int totalEnemies;
    float spawnInterval;
    float preDelay;
    int enemyType;

    struct EnemyGroup {
        int count;
        int typeIndex;
        float delayBefore;
    };
    std::vector<EnemyGroup> groups;
};

// ---- Game Constants ----
namespace GameConst {
    constexpr int GRID_COLS = 20;
    constexpr int GRID_ROWS = 14;
    constexpr float CELL_SIZE = 2.0f;
    constexpr float GRID_OFFSET_X = -(GRID_COLS * CELL_SIZE) * 0.5f;
    constexpr float GRID_OFFSET_Z = -(GRID_ROWS * CELL_SIZE) * 0.5f;

    constexpr int STARTING_GOLD = 300;
    constexpr int CARROT_MAX_HP = 20;
    constexpr float SELL_REFUND_RATIO = 0.6f;

    constexpr float PROJECTILE_SPEED = 25.0f;
    constexpr float PROJECTILE_LIFETIME = 2.5f;
    constexpr int MAX_PROJECTILES = 300;
    constexpr int MAX_ENEMIES = 100;
    constexpr int MAX_TOWERS = 50;

    constexpr int MAX_WAVES = 10;
    constexpr float WAVE_INTERMISSION = 5.0f;

    enum CellType : int {
        Empty = 0,
        Path  = 1,
        Buildable = 2,
        Spawn = 3,
        Carrot = 4,
        Blocked = 5
    };
}

// ---- Tower definitions ----
static const TowerDef TOWER_DEFS[] = {
    // Archer
    { TowerType::Archer, "Archer Tower", 8.0f, 2.5f, 5.0f, 0.0f, 1.0f, 0.0f, 100,
      glm::vec3(0.3f, 0.7f, 0.3f), glm::vec3(0.2f, 1.0f, 0.2f) },
    // Cannon
    { TowerType::Cannon, "Cannon Tower", 7.0f, 1.2f, 8.0f, 2.5f, 1.0f, 0.0f, 150,
      glm::vec3(0.7f, 0.35f, 0.2f), glm::vec3(1.0f, 0.5f, 0.1f) },
    // Ice
    { TowerType::Ice,    "Ice Tower",    7.5f, 1.8f, 2.0f, 0.0f, 0.4f, 2.5f, 120,
      glm::vec3(0.3f, 0.5f, 0.9f), glm::vec3(0.4f, 0.7f, 1.0f) },
    // Magic
    { TowerType::Magic,  "Magic Tower",  9.0f, 1.5f, 12.0f, 0.0f, 1.0f, 0.0f, 200,
      glm::vec3(0.6f, 0.2f, 0.7f), glm::vec3(0.9f, 0.3f, 1.0f) },
};

static const int NUM_TOWER_TYPES = sizeof(TOWER_DEFS) / sizeof(TOWER_DEFS[0]);

// ---- Enemy definitions ----
static const EnemyDef ENEMY_DEFS[] = {
    // 0: Basic
    { "Basic",   3.0f,  5.0f,  1.0f, 0.35f, 1.0f, glm::vec3(0.9f,0.25f,0.2f),   15, 10 },
    // 1: Fast
    { "Fast",    5.5f,  3.0f,  1.0f, 0.30f, 0.8f, glm::vec3(0.9f,0.7f,0.1f),    20, 20 },
    // 2: Tank
    { "Tank",    2.0f, 12.0f,  3.0f, 0.55f, 1.5f, glm::vec3(0.5f,0.15f,0.6f),   35, 30 },
    // 3: Boss
    { "Boss",    2.5f, 50.0f,  5.0f, 0.70f, 2.2f, glm::vec3(0.95f,0.1f,0.05f), 100, 100 },
};

static const int NUM_ENEMY_TYPES = sizeof(ENEMY_DEFS) / sizeof(ENEMY_DEFS[0]);
