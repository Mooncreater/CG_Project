#pragma once

#include <cstdint>
#include <vector>

#include <glm/glm.hpp>
#include "../base/vertex.h"

enum class PrimitiveType {
    Cube,
    Sphere,
    Cylinder,
    Cone,
    Prism,
    Frustum
};

struct Element {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    PrimitiveType type;
};

class Primitives {
public:
    static Element CreateCube(float size);
    static Element CreateSphere(float radius, int segments);
    static Element CreateCylinder(float radius, float height, int segments);
    static Element CreateCone(float radius, float height, int segments);
    static Element CreatePrism(int sides, float radius, float height);
    static Element CreateFrustum(int sides, float r1, float r2, float height);
};
