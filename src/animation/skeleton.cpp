#include "skeleton.h"
#include <algorithm>
#include <stdexcept>

void Skeleton::addBone(const std::string& name, int parent, const glm::mat4& offset) {
    if (parent >= (int)_bones.size() && parent != -1)
        throw std::runtime_error("Skeleton::addBone: invalid parent");
    Bone b;
    b.name = name;
    b.parentIndex = parent;
    b.offsetMatrix = offset;
    _bones.push_back(b);
}

int Skeleton::findBone(const std::string& name) const {
    for (int i = 0; i < (int)_bones.size(); i++)
        if (_bones[i].name == name) return i;
    return -1;
}

void Skeleton::updateMatrices() {
    int n = (int)_bones.size();
    // Build topological order (parents before children)
    _children.clear();
    _children.reserve(n);
    // Simple insertion: bones are added in parent-before-child order
    for (int i = 0; i < n; i++) _children.push_back(i);

    _finalCache.resize(n);
    for (int i = 0; i < n; i++) {
        int idx = _children[i];
        Bone& b = _bones[idx];
        if (b.parentIndex < 0) {
            b.globalTransform = b.localTransform;
        } else {
            b.globalTransform = _bones[b.parentIndex].globalTransform * b.localTransform;
        }
        b.finalMatrix = b.globalTransform * b.offsetMatrix;
        _finalCache[idx] = b.finalMatrix;
    }
}

glm::mat4 Skeleton::boneMatrix(int i) const {
    if (i < 0 || i >= (int)_finalCache.size()) return glm::mat4(1.0f);
    return _finalCache[i];
}
