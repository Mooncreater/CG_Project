#pragma once

#include "types.h"
#include "enemy.h"
#include <vector>

class WaveManager {
public:
    WaveManager();
    void initWaves();
    int update(float deltaTime, EnemySystem& enemySystem);
    void startNextWave();

    bool allWavesComplete() const { return _currentWave >= (int)_waves.size() && _waveDone; }
    bool isWaveActive() const { return _waveActive; }
    int currentWave() const { return _currentWave + 1; }
    int totalWaves() const { return (int)_waves.size(); }
    int enemiesRemaining() const;

private:
    std::vector<WaveDef> _waves;
    int _currentWave = 0;
    bool _waveActive = false;
    bool _waveDone = false;
    float _spawnTimer = 0;
    float _preDelayTimer = 0;
    int _toSpawn = 0;
};

// Global: just use the inline array
inline std::vector<WaveDef> GenerateWaves() {
    std::vector<WaveDef> waves;
    // Wave 1: 6 basic
    { WaveDef w; w.totalEnemies = 6; w.spawnInterval = 1.2f; w.preDelay = 1.0f;
      w.enemyType = 0; waves.push_back(w); }
    // Wave 2: 8 basic + 2 fast
    { WaveDef w; w.totalEnemies = 10; w.spawnInterval = 1.0f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({8, 0, 0});
      w.groups.push_back({2, 1, 1.5f});
      waves.push_back(w); }
    // Wave 3: 10 basic + 2 fast
    { WaveDef w; w.totalEnemies = 12; w.spawnInterval = 0.9f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({10, 0, 0});
      w.groups.push_back({2, 1, 1.2f});
      waves.push_back(w); }
    // Wave 4: 8 basic + 3 tank
    { WaveDef w; w.totalEnemies = 11; w.spawnInterval = 1.0f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({8, 0, 0});
      w.groups.push_back({3, 2, 2.0f});
      waves.push_back(w); }
    // Wave 5: 10 basic + 3 fast + 2 tank
    { WaveDef w; w.totalEnemies = 15; w.spawnInterval = 0.8f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({10, 0, 0});
      w.groups.push_back({3, 1, 1.5f});
      w.groups.push_back({2, 2, 2.5f});
      waves.push_back(w); }
    // Wave 6: 8 basic + 4 fast + 3 tank
    { WaveDef w; w.totalEnemies = 15; w.spawnInterval = 0.75f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({8, 0, 0});
      w.groups.push_back({4, 1, 1.5f});
      w.groups.push_back({3, 2, 2.0f});
      waves.push_back(w); }
    // Wave 7: 6 fast + 5 tank
    { WaveDef w; w.totalEnemies = 11; w.spawnInterval = 0.7f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({6, 1, 0});
      w.groups.push_back({5, 2, 1.0f});
      waves.push_back(w); }
    // Wave 8: 10 fast + 4 tank
    { WaveDef w; w.totalEnemies = 14; w.spawnInterval = 0.65f; w.preDelay = 2.0f;
      w.enemyType = 0;
      w.groups.push_back({10, 1, 0});
      w.groups.push_back({4, 2, 1.5f});
      waves.push_back(w); }
    // Wave 9: 5 fast + 6 tank + 2 boss
    { WaveDef w; w.totalEnemies = 13; w.spawnInterval = 0.6f; w.preDelay = 3.0f;
      w.enemyType = 0;
      w.groups.push_back({5, 1, 0});
      w.groups.push_back({6, 2, 1.5f});
      w.groups.push_back({2, 3, 3.0f});
      waves.push_back(w); }
    // Wave 10: 8 tank + 3 boss
    { WaveDef w; w.totalEnemies = 11; w.spawnInterval = 0.9f; w.preDelay = 3.0f;
      w.enemyType = 0;
      w.groups.push_back({8, 2, 0});
      w.groups.push_back({3, 3, 2.5f});
      waves.push_back(w); }
    return waves;
}
