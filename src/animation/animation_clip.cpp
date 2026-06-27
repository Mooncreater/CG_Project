#include "animation_clip.h"
#include "skeleton.h"
#include <algorithm>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>

const BoneTrack* AnimationClip::findTrack(int boneIndex) const {
    for (auto& t : tracks)
        if (t.boneIndex == boneIndex) return &t;
    return nullptr;
}

BoneTrack* AnimationClip::findTrack(int boneIndex) {
    for (auto& t : tracks)
        if (t.boneIndex == boneIndex) return &t;
    return nullptr;
}

BoneKeyframe AnimationClip::interpolate(const BoneKeyframe& a, const BoneKeyframe& b, float t) {
    BoneKeyframe r;
    r.time = a.time + t * (b.time - a.time);
    r.translation = glm::mix(a.translation, b.translation, t);
    r.rotation = glm::slerp(a.rotation, b.rotation, t);
    r.scale = glm::mix(a.scale, b.scale, t);
    return r;
}

void AnimationClip::sampleBone(int boneIndex, float time,
                                glm::vec3& outPos, glm::quat& outRot, glm::vec3& outScl) const {
    const BoneTrack* track = findTrack(boneIndex);
    if (!track || track->keyframes.empty()) {
        outPos = glm::vec3(0); outRot = glm::quat(1,0,0,0); outScl = glm::vec3(1);
        return;
    }
    if (track->keyframes.size() == 1) {
        auto& k = track->keyframes[0];
        outPos = k.translation; outRot = k.rotation; outScl = k.scale;
        return;
    }
    // Clamp time to [0, duration]
    float ct = std::fmod(time, duration);
    if (ct < 0) ct += duration;

    // Find surrounding keyframes
    int i0 = 0;
    for (int i = (int)track->keyframes.size()-1; i >= 0; i--) {
        if (track->keyframes[i].time <= ct) { i0 = i; break; }
    }
    int i1 = (i0 + 1) % track->keyframes.size();
    if (i1 == 0 && ct > track->keyframes.back().time)
        i1 = i0; // at end, hold last frame

    auto& k0 = track->keyframes[i0];
    auto& k1 = track->keyframes[i1];

    float dur = k1.time - k0.time;
    float t = (dur > 0.001f) ? (ct - k0.time) / dur : 0.0f;
    if(t<0)t=0;if(t>1)t=1;

    auto kr = interpolate(k0, k1, t);
    outPos = kr.translation; outRot = kr.rotation; outScl = kr.scale;
}

void AnimationClip::applyToSkeleton(Skeleton& skel, float time) const {
    for (int i = 0; i < skel.boneCount(); i++) {
        glm::vec3 pos; glm::quat rot; glm::vec3 scl;
        sampleBone(i, time, pos, rot, scl);
        glm::mat4 T = glm::translate(glm::mat4(1.0f), pos);
        glm::mat4 R = glm::toMat4(rot);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scl);
        skel.bone(i).localTransform = T * R * S;
    }
    skel.updateMatrices();
}
