#pragma once

#include <vector>
#include <array>
#include <glm/glm.hpp>

// ============================================================
struct ConvexHull {
    std::vector<glm::vec3> vertices;

    glm::vec3 support(const glm::vec3& dir) const {
        float best = -1e30f;
        int bestIdx = 0;
        for (size_t i = 0; i < vertices.size(); i++) {
            float d = glm::dot(vertices[i], dir);
            if (d > best) { best = d; bestIdx = (int)i; }
        }
        return vertices[bestIdx];
    }

    static ConvexHull fromVertices(const std::vector<glm::vec3>& pts, int samples = 42) {
        ConvexHull hull;
        if (pts.empty()) return hull;
        const float phi = 1.61803398875f;
        for (int i = 0; i < samples; i++) {
            float y = 1.0f - (2.0f * i + 1.0f) / samples;
            float r = sqrtf(1.0f - y * y);
            float theta = 2.0f * 3.14159265f * i / phi;
            glm::vec3 dir(r * cosf(theta), y, r * sinf(theta));
            float best = -1e30f;
            glm::vec3 bestPt;
            for (const auto& p : pts) {
                float d = glm::dot(p, dir);
                if (d > best) { best = d; bestPt = p; }
            }
            bool dup = false;
            for (const auto& v : hull.vertices) {
                if (glm::distance(v, bestPt) < 0.0001f) { dup = true; break; }
            }
            if (!dup) hull.vertices.push_back(bestPt);
        }
        return hull;
    }

    // Build from a cylinder: center at (cx, cz), y from yBottom to yTop, radius r
    static ConvexHull fromCylinder(float cx, float cz,
                                   float yBottom, float yTop, float radius,
                                   int ringSegments = 16) {
        ConvexHull hull;
        // Bottom ring + top ring
        for (int i = 0; i < ringSegments; i++) {
            float angle = 2.0f * 3.14159265f * i / ringSegments;
            float px = cx + cosf(angle) * radius;
            float pz = cz + sinf(angle) * radius;
            hull.vertices.push_back(glm::vec3(px, yBottom, pz));
            hull.vertices.push_back(glm::vec3(px, yTop, pz));
        }
        return hull;
    }
};

// ============================================================
struct Capsule {
    glm::vec3 base, tip;
    float radius;

    glm::vec3 support(const glm::vec3& dir) const {
        float len2 = glm::dot(dir, dir);
        if (len2 < 1e-12f) return base;
        glm::vec3 ndir = dir / sqrtf(len2);
        glm::vec3 segPt = glm::dot(dir, tip) > glm::dot(dir, base) ? tip : base;
        return segPt + ndir * radius;
    }
};

// ============================================================
namespace GJK {

    struct Simplex {
        std::array<glm::vec3, 4> pts;
        int n = 0;
        void push(const glm::vec3& p) { pts[n++] = p; }
    };

    inline glm::vec3 supportAB(const ConvexHull& A, const ConvexHull& B,
                                const glm::vec3& dir) {
        return A.support(dir) - B.support(-dir);
    }
    inline glm::vec3 supportAB(const Capsule& A, const ConvexHull& B,
                                const glm::vec3& dir) {
        return A.support(dir) - B.support(-dir);
    }
    inline glm::vec3 supportAB(const ConvexHull& A, const Capsule& B,
                                const glm::vec3& dir) {
        return A.support(dir) - B.support(-dir);
    }

    // --- simplex reduction ---
    inline bool update2(Simplex& s, glm::vec3& dir) {
        glm::vec3 a = s.pts[1], b = s.pts[0];
        glm::vec3 ab = b - a, ao = -a;
        if (glm::dot(ab, ao) > 0) {
            dir = glm::cross(glm::cross(ab, ao), ab);
        } else {
            s.pts[0] = a; s.n = 1; dir = ao;
        }
        return false;
    }

    inline bool update3(Simplex& s, glm::vec3& dir) {
        glm::vec3 a = s.pts[2], b = s.pts[1], c = s.pts[0];
        glm::vec3 ao = -a;
        glm::vec3 ab = b - a, ac = c - a;
        glm::vec3 abc = glm::cross(ab, ac);

        if (glm::dot(glm::cross(abc, ac), ao) > 0) {
            if (glm::dot(ac, ao) > 0) {
                s.pts[1] = a; s.pts[0] = c; s.n = 2;
                dir = glm::cross(glm::cross(ac, ao), ac);
            } else goto check_ab;
        } else {
            if (glm::dot(glm::cross(ab, abc), ao) > 0) {
            check_ab:
                if (glm::dot(ab, ao) > 0) {
                    s.pts[1] = a; s.pts[0] = b; s.n = 2;
                    dir = glm::cross(glm::cross(ab, ao), ab);
                } else { s.pts[0] = a; s.n = 1; dir = ao; }
            } else {
                float d = glm::dot(abc, ao);
                if (d > 0) {
                    s.pts[0] = c; s.pts[1] = b; s.pts[2] = a; s.n = 3; dir = abc;
                } else {
                    s.pts[0] = b; s.pts[1] = c; s.pts[2] = a; s.n = 3; dir = -abc;
                }
            }
        }
        return false;
    }

    inline bool update4(Simplex& s, glm::vec3&) {
        glm::vec3 a = s.pts[3], b = s.pts[2], c = s.pts[1], d = s.pts[0];
        glm::vec3 ao = -a;
        glm::vec3 ab = b - a, ac = c - a, ad = d - a;

        glm::vec3 abc = glm::cross(ab, ac);
        if (glm::dot(abc, ao) > 0) {
            s.pts[0]=c; s.pts[1]=b; s.pts[2]=a; s.n=3; glm::vec3 dum; update3(s, dum); return false;
        }
        glm::vec3 acd = glm::cross(ac, ad);
        if (glm::dot(acd, ao) > 0) {
            s.pts[0]=d; s.pts[1]=c; s.pts[2]=a; s.n=3; glm::vec3 dum; update3(s, dum); return false;
        }
        glm::vec3 adb = glm::cross(ad, ab);
        if (glm::dot(adb, ao) > 0) {
            s.pts[0]=b; s.pts[1]=d; s.pts[2]=a; s.n=3; glm::vec3 dum; update3(s, dum); return false;
        }
        return true;  // origin inside tetrahedron
    }

    template<typename ShapeA, typename ShapeB>
    bool intersect(const ShapeA& A, const ShapeB& B, int maxIter = 32) {
        glm::vec3 dir = glm::vec3(1, 0, 0);
        Simplex simplex;
        simplex.push(supportAB(A, B, dir));
        dir = -simplex.pts[0];

        for (int iter = 0; iter < maxIter; iter++) {
            glm::vec3 p = supportAB(A, B, dir);
            if (glm::dot(p, dir) < 0) return false;
            simplex.push(p);
            bool collision = false;
            switch (simplex.n) {
                case 2: collision = update2(simplex, dir); break;
                case 3: collision = update3(simplex, dir); break;
                case 4: collision = update4(simplex, dir); break;
            }
            if (collision) return true;
        }
        return false;
    }

} // namespace GJK
