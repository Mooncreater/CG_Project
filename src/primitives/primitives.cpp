#include "primitives.h"
#include <cmath>

// ---- helpers ----

static void addFace(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices,
                    glm::vec3 v0, glm::vec3 v1, glm::vec3 v2, glm::vec3 v3,
                    glm::vec3 normal) {
    uint32_t base = static_cast<uint32_t>(vertices.size());
    vertices.emplace_back(v0, normal, glm::vec2(0.0f, 0.0f));
    vertices.emplace_back(v1, normal, glm::vec2(1.0f, 0.0f));
    vertices.emplace_back(v2, normal, glm::vec2(1.0f, 1.0f));
    vertices.emplace_back(v3, normal, glm::vec2(0.0f, 1.0f));
    indices.insert(indices.end(), { base, base + 1, base + 2, base, base + 2, base + 3 });
}

// ---- Cube ----

Element Primitives::CreateCube(float size) {
    Element e;
    e.type = PrimitiveType::Cube;
    float half = size * 0.5f;

    addFace(e.vertices, e.indices,
        { half, -half,  half }, { half, -half, -half },
        { half,  half, -half }, { half,  half,  half },
        { 1.0f, 0.0f, 0.0f });
    addFace(e.vertices, e.indices,
        {-half, -half, -half }, {-half, -half,  half },
        {-half,  half,  half }, {-half,  half, -half },
        {-1.0f, 0.0f, 0.0f });
    addFace(e.vertices, e.indices,
        {-half,  half,  half }, { half,  half,  half },
        { half,  half, -half }, {-half,  half, -half },
        { 0.0f, 1.0f, 0.0f });
    addFace(e.vertices, e.indices,
        {-half, -half, -half }, { half, -half, -half },
        { half, -half,  half }, {-half, -half,  half },
        { 0.0f, -1.0f, 0.0f });
    addFace(e.vertices, e.indices,
        {-half, -half,  half }, { half, -half,  half },
        { half,  half,  half }, {-half,  half,  half },
        { 0.0f, 0.0f, 1.0f });
    addFace(e.vertices, e.indices,
        { half, -half, -half }, {-half, -half, -half },
        {-half,  half, -half }, { half,  half, -half },
        { 0.0f, 0.0f, -1.0f });

    return e;
}

// ---- Sphere (UV sphere) ----

Element Primitives::CreateSphere(float radius, int segments) {
    Element e;
    e.type = PrimitiveType::Sphere;
    int stacks = segments;
    int slices = segments;

    for (int i = 0; i <= stacks; ++i) {
        float phi = glm::pi<float>() * float(i) / float(stacks); // 0 to PI
        float y = radius * std::cos(phi);
        float r = radius * std::sin(phi);

        for (int j = 0; j <= slices; ++j) {
            float theta = 2.0f * glm::pi<float>() * float(j) / float(slices);
            float x = r * std::cos(theta);
            float z = r * std::sin(theta);
            glm::vec3 pos(x, y, z);
            glm::vec3 normal = glm::normalize(pos);
            float u = float(j) / float(slices);
            float v = float(i) / float(stacks);
            e.vertices.emplace_back(pos, normal, glm::vec2(u, v));
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            uint32_t a = uint32_t(i * (slices + 1) + j);
            uint32_t b = a + uint32_t(slices + 1);
            e.indices.insert(e.indices.end(), { a, b, a + 1, b, b + 1, a + 1 });
        }
    }

    return e;
}

// ---- Cylinder (centered on Y) ----

Element Primitives::CreateCylinder(float radius, float height, int segments) {
    Element e;
    e.type = PrimitiveType::Cylinder;
    float halfH = height * 0.5f;

    // top center + ring, bottom center + ring
    for (int j = 0; j <= segments; ++j) {
        float theta = 2.0f * glm::pi<float>() * float(j) / float(segments);
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);

        // top ring vertex
        e.vertices.emplace_back(glm::vec3(x, halfH, z), glm::vec3(0, 1, 0), glm::vec2(0, 0));
        // bottom ring vertex
        e.vertices.emplace_back(glm::vec3(x, -halfH, z), glm::vec3(0, -1, 0), glm::vec2(0, 0));
    }

    // side faces
    for (int j = 0; j < segments; ++j) {
        uint32_t t0 = uint32_t(j * 2);
        uint32_t b0 = t0 + 1;
        uint32_t t1 = t0 + 2;
        uint32_t b1 = t0 + 3;
        e.indices.insert(e.indices.end(), { t0, b0, t1, b0, b1, t1 });
    }

    // top cap
    uint32_t topCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, halfH, 0), glm::vec3(0, 1, 0), glm::vec2(0, 0));
    for (int j = 0; j < segments; ++j) {
        uint32_t cur = uint32_t(j * 2);
        uint32_t nxt = uint32_t(((j + 1) % segments) * 2);
        e.indices.insert(e.indices.end(), { topCenter, nxt, cur });
    }

    // bottom cap
    uint32_t botCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, -halfH, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0));
    for (int j = 0; j < segments; ++j) {
        uint32_t cur = uint32_t(j * 2 + 1);
        uint32_t nxt = uint32_t(((j + 1) % segments) * 2 + 1);
        e.indices.insert(e.indices.end(), { botCenter, cur, nxt });
    }

    return e;
}

// ---- Cone (apex at +Y, base at -Y) ----

Element Primitives::CreateCone(float radius, float height, int segments) {
    Element e;
    e.type = PrimitiveType::Cone;
    float halfH = height * 0.5f;

    // base ring
    for (int j = 0; j <= segments; ++j) {
        float theta = 2.0f * glm::pi<float>() * float(j) / float(segments);
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        e.vertices.emplace_back(glm::vec3(x, -halfH, z), glm::vec3(0, 0, 0), glm::vec2(0, 0));
    }

    // apex
    uint32_t apex = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, halfH, 0), glm::vec3(0, 0, 0), glm::vec2(0, 0));

    // side triangles
    for (int j = 0; j < segments; ++j) {
        uint32_t a = uint32_t(j);
        uint32_t b = uint32_t(j + 1);
        // compute smooth normal for the face
        glm::vec3 p0 = e.vertices[a].position;
        glm::vec3 p1 = e.vertices[b].position;
        glm::vec3 pa = e.vertices[apex].position;
        glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, pa - p0));
        e.vertices[a].normal = normal;
        e.vertices[b].normal = normal;
        e.vertices[apex].normal = normal;
        e.indices.insert(e.indices.end(), { a, b, apex });
    }

    // bottom cap
    uint32_t botCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, -halfH, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0));
    for (int j = 0; j < segments; ++j) {
        uint32_t cur = uint32_t(j);
        uint32_t nxt = uint32_t((j + 1) % segments);
        e.indices.insert(e.indices.end(), { botCenter, nxt, cur });
    }

    return e;
}

// ---- Prism (regular N-gon prism centered on Y) ----

Element Primitives::CreatePrism(int sides, float radius, float height) {
    Element e;
    e.type = PrimitiveType::Prism;
    float halfH = height * 0.5f;

    // top + bottom rings
    for (int j = 0; j <= sides; ++j) {
        float theta = 2.0f * glm::pi<float>() * float(j) / float(sides);
        float x = radius * std::cos(theta);
        float z = radius * std::sin(theta);
        e.vertices.emplace_back(glm::vec3(x, halfH, z), glm::vec3(0, 1, 0), glm::vec2(0, 0));
        e.vertices.emplace_back(glm::vec3(x, -halfH, z), glm::vec3(0, -1, 0), glm::vec2(0, 0));
    }

    // side quads
    for (int j = 0; j < sides; ++j) {
        uint32_t t0 = uint32_t(j * 2);
        uint32_t b0 = t0 + 1;
        uint32_t t1 = t0 + 2;
        uint32_t b1 = t0 + 3;
        e.indices.insert(e.indices.end(), { t0, b0, t1, b0, b1, t1 });
    }

    // top cap (triangle fan)
    uint32_t topCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, halfH, 0), glm::vec3(0, 1, 0), glm::vec2(0, 0));
    for (int j = 0; j < sides; ++j) {
        uint32_t cur = uint32_t(j * 2);
        uint32_t nxt = uint32_t(((j + 1) % sides) * 2);
        e.indices.insert(e.indices.end(), { topCenter, nxt, cur });
    }

    // bottom cap
    uint32_t botCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, -halfH, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0));
    for (int j = 0; j < sides; ++j) {
        uint32_t cur = uint32_t(j * 2 + 1);
        uint32_t nxt = uint32_t(((j + 1) % sides) * 2 + 1);
        e.indices.insert(e.indices.end(), { botCenter, cur, nxt });
    }

    return e;
}

// ---- Frustum (regular N-gon pyramid frustum centered on Y) ----

Element Primitives::CreateFrustum(int sides, float r1, float r2, float height) {
    Element e;
    e.type = PrimitiveType::Frustum;
    float halfH = height * 0.5f;

    // bottom ring (r1) + top ring (r2)
    for (int j = 0; j <= sides; ++j) {
        float theta = 2.0f * glm::pi<float>() * float(j) / float(sides);
        float cx = std::cos(theta);
        float cz = std::sin(theta);
        e.vertices.emplace_back(glm::vec3(r1 * cx, -halfH, r1 * cz), glm::vec3(0, 0, 0), glm::vec2(0, 0));
        e.vertices.emplace_back(glm::vec3(r2 * cx,  halfH, r2 * cz), glm::vec3(0, 0, 0), glm::vec2(0, 0));
    }

    // side quads
    for (int j = 0; j < sides; ++j) {
        uint32_t b0 = uint32_t(j * 2);
        uint32_t t0 = b0 + 1;
        uint32_t b1 = b0 + 2;
        uint32_t t1 = b0 + 3;
        e.indices.insert(e.indices.end(), { b0, t0, b1, t0, t1, b1 });
    }

    // bottom cap
    uint32_t botCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, -halfH, 0), glm::vec3(0, -1, 0), glm::vec2(0, 0));
    for (int j = 0; j < sides; ++j) {
        uint32_t cur = uint32_t(j * 2);
        uint32_t nxt = uint32_t(((j + 1) % sides) * 2);
        e.indices.insert(e.indices.end(), { botCenter, nxt, cur });
    }

    // top cap
    uint32_t topCenter = uint32_t(e.vertices.size());
    e.vertices.emplace_back(glm::vec3(0, halfH, 0), glm::vec3(0, 1, 0), glm::vec2(0, 0));
    for (int j = 0; j < sides; ++j) {
        uint32_t cur = uint32_t(j * 2 + 1);
        uint32_t nxt = uint32_t(((j + 1) % sides) * 2 + 1);
        e.indices.insert(e.indices.end(), { topCenter, cur, nxt });
    }

    return e;
}
