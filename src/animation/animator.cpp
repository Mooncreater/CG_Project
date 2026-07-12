#include "animator.h"
#include <cmath>

Animator::Animator(Skeleton& skeleton) : _skeleton(skeleton) {}

void Animator::play(const AnimationClip* clip, bool loop) {
    _fromClip = nullptr;
    _blendLeft = 0;
    _clip = clip;
    _time = 0;
    _playing = true;
    _loop = loop;
    if (_clip) {
        _clip->applyToSkeleton(_skeleton, 0);
    }
}

void Animator::stop() {
    _playing = false;
    _time = 0;
    _clip = nullptr;
    _fromClip = nullptr;
}

void Animator::pause() {
    _playing = false;
}

void Animator::resume() {
    if (_clip) {
        _playing = true;
    }
}

void Animator::crossFade(const AnimationClip* clip, float fadeTime) {
    if (!_clip || !clip) {
        play(clip, clip ? clip->looping : false);
        return;
    }

    _fromClip = _clip;
    _fromTime = _time;
    _blendDur = fadeTime;
    _blendLeft = fadeTime;
    _clip = clip;
    _time = 0;
    _playing = true;
    _loop = clip ? clip->looping : false;
    _clip->applyToSkeleton(_skeleton, 0);

    // Immediately blend back to current state, then blend will transition
    applyBlended(0.0f);
}

void Animator::applyBlended(float blendT) {
    // blendT: 0 = fully fromClip, 1 = fully current clip
    if (!_fromClip || blendT >= 1.0f) {
        if (_clip) {
            _clip->applyToSkeleton(_skeleton, _time);
        }

        return;
    }
    if (blendT <= 0.0f) {
        if (_fromClip) {
            _fromClip->applyToSkeleton(_skeleton, _fromTime);
        }

        return;
    }

    // Create temp skeleton to sample the "from" clip
    Skeleton tmp;
    // Copy bone structure from _skeleton
    for (int i = 0; i < _skeleton.boneCount(); i++) {
        auto& b = _skeleton.bone(i);
        tmp.addBone(b.name, b.parentIndex, b.offsetMatrix);
    }
    // Apply "from" clip to temp skeleton
    _fromClip->applyToSkeleton(tmp, _fromTime);

    // Apply current clip to main skeleton
    _clip->applyToSkeleton(_skeleton, _time);

    // Blend: lerp each bone's localTransform
    for (int i = 0; i < _skeleton.boneCount() && i < tmp.boneCount(); i++) {
        auto& a = tmp.bone(i);
        auto& b = _skeleton.bone(i);
        // Decompose both, lerp, recompose
        // Simple approach: lerp the 4x4 matrices (approximate, works for small differences)
        // Better: extract translation and rotation, slerp rotation
        glm::vec3 ta = glm::vec3(a.localTransform[3]);
        glm::vec3 tb = glm::vec3(b.localTransform[3]);
        glm::vec3 tl = glm::mix(ta, tb, blendT);

        // Extract rotation (approximate)
        glm::quat qa = glm::quat_cast(glm::mat3(a.localTransform));
        glm::quat qb = glm::quat_cast(glm::mat3(b.localTransform));
        glm::quat ql = glm::slerp(qa, qb, blendT);

        // Extract scale
        glm::vec3 sa(glm::length(glm::vec3(a.localTransform[0])),
            glm::length(glm::vec3(a.localTransform[1])),
            glm::length(glm::vec3(a.localTransform[2])));
        glm::vec3 sb(glm::length(glm::vec3(b.localTransform[0])),
            glm::length(glm::vec3(b.localTransform[1])),
            glm::length(glm::vec3(b.localTransform[2])));
        glm::vec3 sl = glm::mix(sa, sb, blendT);

        b.localTransform = glm::translate(glm::mat4(1.0f), tl)
            * glm::mat4_cast(ql)
            * glm::scale(glm::mat4(1.0f), sl);
    }
    _skeleton.updateMatrices();
}

bool Animator::update(float deltaTime) {
    if (!_playing || !_clip) return false;
    _time += deltaTime * _speed;

    // Update cross-fade
    if (_blendLeft > 0) {
        _blendLeft -= deltaTime;
        if (_fromClip) {
            _fromTime += deltaTime * _speed;
        }

        if (_blendLeft <= 0) {
            _fromClip = nullptr;
            _blendLeft = 0;
            _clip->applyToSkeleton(_skeleton, _time);
        }
        else {
            float t = 1.0f - (_blendLeft / _blendDur);
            applyBlended(t);
        }

        if (_time >= _clip->duration && !_loop) {
            _playing = false;
            return true;
        }
        
        return false;
    }

    bool ended = false;
    if (_time >= _clip->duration) {
        if (_loop) {
            _time = std::fmod(_time, _clip->duration);
        }
        else {
            _time = _clip->duration;
            _playing = false;
            ended = true;
        }
    }
    _clip->applyToSkeleton(_skeleton, _time);

    return ended;
}

void Animator::seek(float t) {
    if (_clip) {
        _time = (t < 0 ? 0 : (t > _clip->duration ? _clip->duration : t));
        _clip->applyToSkeleton(_skeleton, _time);
    }
}
