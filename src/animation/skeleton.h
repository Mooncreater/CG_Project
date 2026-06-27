#pragma once
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

// A single bone in the skeleton hierarchy
struct Bone {
    std::string name;
    int parentIndex = -1;          // -1 = root
    glm::mat4 offsetMatrix{1.0f};  // inverse bind pose (model space → bone local)
    glm::mat4 localTransform{1.0f};// TRS relative to parent
    glm::mat4 globalTransform{1.0f};// accumulated from root
    glm::mat4 finalMatrix{1.0f};   // globalTransform * offsetMatrix → shader
};

// Skeleton: a hierarchy of bones
class Skeleton {
public:
    void addBone(const std::string& name, int parent, const glm::mat4& offset);

    Bone& bone(int i) { return _bones[i]; }
    const Bone& bone(int i) const { return _bones[i]; }
    int boneCount() const { return (int)_bones.size(); }
    int findBone(const std::string& name) const;

    // Recurse from root to compute global and final matrices
    void updateMatrices();

    const std::vector<glm::mat4>& finalMatrices() const { return _finalCache; }
    glm::mat4 boneMatrix(int i) const;

private:
    std::vector<Bone> _bones;
    std::vector<glm::mat4> _finalCache;
    std::vector<int> _children;    // flattened parent→child order
    void updateRecurse(int idx, const glm::mat4& parentGlobal);
};
