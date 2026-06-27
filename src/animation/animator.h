#pragma once
#include "animation_clip.h"
#include "skeleton.h"
#include <memory>

class Animator {
public:
    Animator(Skeleton& skeleton);

    void play(const AnimationClip* clip, bool loop = true);
    void stop(); void pause(); void resume();
    bool update(float deltaTime);

    float currentTime() const { return _time; }
    float duration() const { return _clip ? _clip->duration : 0.0f; }
    bool isPlaying() const { return _playing; }
    const AnimationClip* currentClip() const { return _clip; }
    void seek(float t);

    // Smooth transition: blend from current clip to new clip over fadeTime seconds
    void crossFade(const AnimationClip* clip, float fadeTime);

private:
    Skeleton& _skeleton;
    const AnimationClip* _clip = nullptr;
    float _time = 0.0f, _speed = 1.0f;
    bool _playing = false, _loop = false;

    // Blend state
    const AnimationClip* _fromClip = nullptr;
    float _fromTime = 0.0f, _blendLeft = 0.0f, _blendDur = 0.0f;

    void applyBlended(float blendT);
};
