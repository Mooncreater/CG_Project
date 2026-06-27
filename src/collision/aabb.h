#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <algorithm>

struct AABB {
    glm::vec3 min, max;

    AABB() = default;
    AABB(const glm::vec3& c, const glm::vec3& he) : min(c - he), max(c + he) {}
    static AABB fromMinMax(const glm::vec3& mi, const glm::vec3& ma) { AABB b; b.min=mi; b.max=ma; return b; }

    bool overlaps(const AABB& o) const {
        return min.x < o.max.x && max.x > o.min.x &&
               min.y < o.max.y && max.y > o.min.y &&
               min.z < o.max.z && max.z > o.min.z;
    }
};

// ============================================================
// Per-axis collision test + resolution (Minecraft-style)
// ============================================================
// Moves `box` by `delta` on each axis independently.
// On each axis, if the swept movement hits a solid block, stops at the
// block boundary (plus a tiny epsilon to stay outside).
// This produces natural wall-sliding and prevents tunneling.
// Returns the resolved movement delta (may be less than requested).
// Also sets `onGround` if there is a block directly under the box.
// isSolid(blockValue) → bool (optional; all blocks solid by default)

namespace detail {
template<typename BlockMap, typename SolidPred>
glm::vec3 collideAndSlideImpl(const AABB& box, const glm::vec3& delta,
                              const BlockMap& blocks, bool& onGround,
                              SolidPred isSolid) {
    glm::vec3 result(0);
    onGround = false;
    const float eps = 0.001f;

    auto boxOverlapsBlocks = [&](const AABB& b) {
        int x0 = (int)floorf(b.min.x);
        int x1 = (int)floorf(b.max.x - eps) + 1;
        int y0 = (int)floorf(b.min.y);
        int y1 = (int)floorf(b.max.y - eps) + 1;
        int z0 = (int)floorf(b.min.z);
        int z1 = (int)floorf(b.max.z - eps) + 1;
        for (int x = x0; x <= x1; ++x)
            for (int y = y0; y <= y1; ++y)
                for (int z = z0; z <= z1; ++z) {
                    auto it = blocks.find(glm::ivec3(x, y, z));
                    if (it == blocks.end()) continue;
                    if (!isSolid(it->second)) continue;
                    AABB blockBox(glm::vec3(x, y, z), glm::vec3(0.5f));
                    if (b.overlaps(blockBox)) return true;
                }
        return false;
    };

    auto checkAndMove = [&](int axis, float amount) {
        if (fabsf(amount) < eps) return;
        AABB moved = box;
        (&moved.min.x)[axis] += amount;
        (&moved.max.x)[axis] += amount;
        // For Y movement, shrink XZ slightly so side walls don't block jumping
        if (axis == 1) {
            const float margin = 0.12f;
            moved.min.x += margin; moved.max.x -= margin;
            moved.min.z += margin; moved.max.z -= margin;
        }
        if (!boxOverlapsBlocks(moved)) {
            (&result.x)[axis] = amount;
        }
    };

    // Y first (gravity/jump priority)
    checkAndMove(1, delta.y);

    // X and Z (slide along walls)
    checkAndMove(0, delta.x);
    checkAndMove(2, delta.z);

    // Ground check: scan all block positions under and at the player's feet
    float feetY = box.min.y + result.y;
    int byLow  = (int)floorf(feetY - 0.01f);
    int byHigh = (int)floorf(feetY + 0.01f);
    int bx0 = (int)floorf(box.min.x + result.x);
    int bx1 = (int)floorf(box.max.x + result.x - eps) + 1;
    int bz0 = (int)floorf(box.min.z + result.z);
    int bz1 = (int)floorf(box.max.z + result.z - eps) + 1;
    for (int bx = bx0; bx <= bx1; ++bx) {
        for (int bz = bz0; bz <= bz1; ++bz) {
            for (int by = byLow; by <= byHigh; ++by) {
                auto it = blocks.find(glm::ivec3(bx, by, bz));
                if (it != blocks.end() && isSolid(it->second)) {
                    onGround = true;
                    return result;
                }
            }
        }
    }

    return result;
}
} // namespace detail

// All blocks solid (default)
template<typename BlockMap>
glm::vec3 collideAndSlide(const AABB& box, const glm::vec3& delta,
                          const BlockMap& blocks, bool& onGround) {
    return detail::collideAndSlideImpl(box, delta, blocks, onGround,
        [](const auto&) { return true; });
}

// With solid-predicate
template<typename BlockMap, typename SolidPred>
glm::vec3 collideAndSlide(const AABB& box, const glm::vec3& delta,
                          const BlockMap& blocks, bool& onGround,
                          SolidPred isSolid) {
    return detail::collideAndSlideImpl(box, delta, blocks, onGround, isSolid);
}
