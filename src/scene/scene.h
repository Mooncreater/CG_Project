#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "../primitives/primitives.h"

enum class SceneObjType {
    Wall,         // cube - walls, interior dividers
    Window,       // cube - decorative window slit
    Floor,        // cube - ground, ceiling, elevated platforms
    EndPoint,     // sphere - win trigger
    StairStep,    // cube - stair tread
    Tower,        // cylinder - round tower
    ConeRoof,     // cone - tower roof / spire
    Battlement    // small cube - crenellation on wall tops
};

struct AABB {
    glm::vec3 min, max;
    bool intersects(const AABB& other) const;
};

struct SceneObject {
    SceneObjType type;
    glm::vec3 position;
    glm::vec3 size;
    AABB boundingBox;
    glm::vec3 color;
};

class Scene {
public:
    Scene();
    void buildDefaultLevel();

    const std::vector<SceneObject>& objects() const { return _objects; }

private:
    std::vector<SceneObject> _objects;

    void addWall(float x, float z, float w, float h, float d, float yBase = 0.0f);
    void addWindow(float x, float z, float h, float yBase = 0.0f);
    void addFloor(float x, float z, float w, float d,
                  glm::vec3 color = glm::vec3(0.3f, 0.3f, 0.3f), float y = 0.0f);
    void addEndPoint(float x, float z, float yBase = 0.0f);
    void addTower(float x, float z, float radius, float height, float yBase = 0.0f);
    void addConeRoof(float x, float z, float radius, float height, float yBase);
    void addBattlement(float x, float z, float yBase);
    void addStairStep(float x, float z, float w, float d, float topY);
};
