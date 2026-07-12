#pragma once

#include "skeleton.h"
#include "skinned_mesh.h"
#include "animation_clip.h"
#include <memory>
#include <vector>
#include <glm/glm.hpp>

class HumanoidBuilder {
public:
    enum BoneIdx {
        ROOT = 0,
        SPINE = 1,
        HEAD = 2,
        L_UPPER_ARM = 3,
        L_FOREARM = 4,
        R_UPPER_ARM = 5,
        R_FOREARM = 6,
        L_UPPER_LEG = 7,
        L_LOWER_LEG = 8,
        R_UPPER_LEG = 9,
        R_LOWER_LEG = 10,
        BONE_COUNT = 11
    };

public:
    HumanoidBuilder();

    Skeleton& skeleton() { return _skeleton; }

    const Skeleton& skeleton()const { return _skeleton; }

    std::unique_ptr<SkinnedMesh> buildMesh();

    AnimationClip idleClip, waveClip, attackClip, walkClip, jumpClip;

    // Body part index ranges for multi-color rendering
    struct PartRange { uint32_t start, count; };
    PartRange headRange, bodyRange;
    PartRange armL_upper, armL_lower, armR_upper, armR_lower;
    PartRange legL_upper, legL_lower, legR_upper, legR_lower;
    PartRange faceEyes, faceNose, faceMouth, hair;  // facial features

private:
    Skeleton _skeleton;
    std::vector<SkinnedVertex>_vertices;
    std::vector<uint32_t>_indices;

private:
    void buildSkeleton();
    void buildBodyParts();
    void buildAnimations();
    void addBox(float w, float h, float d, int bid, glm::vec3 pos);
    void addSphere(float r, int segs, int bid, glm::vec3 pos);
    void addCyl(float r, float h, int segs, int bid, glm::vec3 pos);

    static std::vector<SkinnedVertex> makeBox(float w, float h, float d, int bid);
    static std::vector<SkinnedVertex> makeSphere(float r, int segs, int bid);
    static std::vector<SkinnedVertex> makeCylinder(float r, float h, int segs, int bid);
    static std::vector<uint32_t> makeBoxIndices(uint32_t base);
    static std::vector<uint32_t> makeSphereIndices(uint32_t base, int segs);
    static std::vector<uint32_t> makeCylinderIndices(uint32_t base, int segs);
};
