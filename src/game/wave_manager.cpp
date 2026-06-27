#include "wave_manager.h"
#include <cstdlib>
#include <algorithm>

WaveManager::WaveManager() {}

void WaveManager::initWaves() {
    _waves = GenerateWaves();
}

void WaveManager::startNextWave() {
    if (_currentWave >= (int)_waves.size()) { _waveDone = true; return; }
    const WaveDef& w = _waves[_currentWave];
    _toSpawn = w.totalEnemies;
    _spawnTimer = 0;
    _preDelayTimer = w.preDelay;
    _waveActive = true;
    _waveDone = false;
}

int WaveManager::update(float deltaTime, EnemySystem& enemySystem) {
    if (!_waveActive) return 0;

    if (_toSpawn <= 0 && enemySystem.aliveCount() == 0) {
        _waveActive = false; _waveDone = true; _currentWave++; return 0;
    }

    if (_toSpawn <= 0) return 0;

    _preDelayTimer -= deltaTime;
    if (_preDelayTimer > 0) return 0;

    _spawnTimer -= deltaTime;
    int spawned = 0;

    while (_spawnTimer <= 0 && _toSpawn > 0) {
        int pathCount = enemySystem.pathCount();
        if (pathCount <= 0) break;

        // Determine enemy type to spawn
        int enemyType = 0;
        const WaveDef& w = _waves[_currentWave];
        int spawnedSoFar = w.totalEnemies - _toSpawn;

        enemyType = w.enemyType; // default
        if (!w.groups.empty()) {
            int cumulative = 0;
            for (const auto& g : w.groups) {
                cumulative += g.count;
                if (spawnedSoFar < cumulative) {
                    enemyType = g.typeIndex;
                    break;
                }
            }
        }

        int pathId = rand() % pathCount;
        if (enemyType >= 0 && enemyType < NUM_ENEMY_TYPES)
            enemySystem.spawnEnemy(pathId, ENEMY_DEFS[enemyType]);
        else
            enemySystem.spawnEnemy(pathId, ENEMY_DEFS[0]);

        _toSpawn--; spawned++;
        _spawnTimer += _waves[_currentWave].spawnInterval;
    }

    return spawned;
}

int WaveManager::enemiesRemaining() const {
    if (_currentWave >= (int)_waves.size()) return 0;
    return _toSpawn;
}
