#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// A single keyframe for one bone at one point in time
struct BoneKeyframe {
    float time = 0.0f;               // seconds
    glm::vec3 translation{0.0f};
    glm::quat rotation{1.0f,0,0,0};  // identity
    glm::vec3 scale{1.0f};
};

// Track: all keyframes for one bone
struct BoneTrack {
    int boneIndex = -1;
    std::vector<BoneKeyframe> keyframes;
};

// Animation clip: named sequence of bone tracks
struct AnimationClip {
    std::string name;
    float duration = 0.0f;           // total seconds
    bool looping = true;
    std::vector<BoneTrack> tracks;

    // Find the track for a given bone index
    const BoneTrack* findTrack(int boneIndex) const;
    BoneTrack* findTrack(int boneIndex);

    // Interpolate a single bone's pose at time t
    void sampleBone(int boneIndex, float time,
                    glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScl) const;

    // Apply the clip at time t to a skeleton (sets localTransform on each bone)
    void applyToSkeleton(class Skeleton& skel, float time) const;

private:
    static BoneKeyframe interpolate(const BoneKeyframe& a, const BoneKeyframe& b, float t);
};
