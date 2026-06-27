#pragma once
#include "../base/vertex.h"
#include "../primitives/primitives.h"
#include <glm/glm.hpp>
#include <vector>
#include <algorithm>
#include <cmath>

namespace NURBS {

inline float basis(int i, int p, float u, const std::vector<float>& knots) {
    if (p == 0) return (u >= knots[i] && u < knots[i + 1]) ? 1.0f : 0.0f;
    float left = 0, right = 0;
    float d1 = knots[i + p] - knots[i];
    float d2 = knots[i + p + 1] - knots[i + 1];
    if (d1 > 1e-8f) left  = (u - knots[i]) / d1 * basis(i, p - 1, u, knots);
    if (d2 > 1e-8f) right = (knots[i + p + 1] - u) / d2 * basis(i + 1, p - 1, u, knots);
    return left + right;
}

inline std::vector<float> makeKnots(int degree, int nCP) {
    int m = nCP + degree + 1;
    std::vector<float> k(m);
    for (int i = 0; i <= degree; i++) { k[i] = 0; k[m - 1 - i] = 1; }
    int inner = nCP - degree;
    for (int i = 1; i < inner; i++) k[degree + i] = (float)i / (float)inner;
    return k;
}

inline glm::vec3 eval(
    float u, float v,
    const std::vector<std::vector<glm::vec3>>& cps,
    const std::vector<std::vector<float>>& weights,
    int dU, int dV,
    const std::vector<float>& kU,
    const std::vector<float>& kV)
{
    int nU = (int)cps[0].size() - 1, nV = (int)cps.size() - 1;
    glm::vec3 num(0); float den = 0;
    for (int j = 0; j <= nV; j++) {
        float Bj = basis(j, dV, v, kV);
        for (int i = 0; i <= nU; i++) {
            float Bi = basis(i, dU, u, kU);
            float w = weights[j][i] * Bi * Bj;
            num += w * cps[j][i]; den += w;
        }
    }
    return den > 1e-10f ? num / den : glm::vec3(0);
}

struct NURBSSurface {
    std::vector<std::vector<glm::vec3>> cp;
    std::vector<std::vector<float>> w;
    int dU = 3, dV = 3;
    std::vector<float> kU, kV;

    Element tessellate(int su = 32, int sv = 32) const {
        Element out;
        int nu = su + 1, nv = sv + 1;
        out.vertices.reserve(nu * nv);
        for (int j = 0; j < nv; j++) {
            float v = (float)j / sv;
            for (int i = 0; i < nu; i++) {
                float u = (float)i / su;
                glm::vec3 p = eval(u, v, cp, w, dU, dV, kU, kV);
                float eps = 0.001f;
                glm::vec3 tu = eval(glm::clamp(u+eps,0.f,1.f), v, cp, w, dU, dV, kU, kV)
                             - eval(glm::clamp(u-eps,0.f,1.f), v, cp, w, dU, dV, kU, kV);
                glm::vec3 tv = eval(u, glm::clamp(v+eps,0.f,1.f), cp, w, dU, dV, kU, kV)
                             - eval(u, glm::clamp(v-eps,0.f,1.f), cp, w, dU, dV, kU, kV);
                glm::vec3 n = glm::normalize(glm::cross(tu, tv));
                Vertex vt; vt.position = p; vt.normal = n; vt.texCoord = glm::vec2(u, v);
                out.vertices.push_back(vt);
            }
        }
        for (int j = 0; j < sv; j++) for (int i = 0; i < su; i++) {
            uint32_t a = j * nu + i, b = a + 1, c = a + nu, d = c + 1;
            out.indices.insert(out.indices.end(), {a,b,d, a,d,c});
        }
        return out;
    }

    static NURBSSurface createPatch() {
        NURBSSurface s; s.dU = s.dV = 3;
        const int N = 5;
        s.cp.resize(N, std::vector<glm::vec3>(N));
        s.w.resize(N, std::vector<float>(N));
        for (int j = 0; j < N; j++) {
            float v = (float)j / (N - 1);
            for (int i = 0; i < N; i++) {
                float u = (float)i / (N - 1);
                s.cp[j][i] = glm::vec3((u - 0.5f) * 6, 1.5f * sinf(u * 3.14f) * cosf(v * 3.14f), (v - 0.5f) * 6);
                s.w[j][i] = 1;
            }
        }
        s.kU = makeKnots(s.dU, N); s.kV = makeKnots(s.dV, N);
        return s;
    }
};

} // namespace NURBS
