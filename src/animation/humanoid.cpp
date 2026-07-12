#include "humanoid.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

HumanoidBuilder::HumanoidBuilder() {
    buildSkeleton();
    buildBodyParts();
    buildAnimations();
}

void HumanoidBuilder::buildSkeleton() {
    auto I = glm::mat4(1.0f);
    auto bl = [&](int b)->glm::mat4 {
        switch (b) {
        case ROOT:return glm::translate(I, { 0, 0, 0 });
        case SPINE:return glm::translate(I, { 0, 0.40f, 0 });
        case HEAD:return glm::translate(I, { 0, 0.55f, 0 });
        case L_UPPER_ARM:return glm::translate(I, { 0.33f, 0.45f, 0 });
        case L_FOREARM:return glm::translate(I, { 0, -0.38f, 0 });
        case R_UPPER_ARM:return glm::translate(I, { -0.33f, 0.45f, 0 });
        case R_FOREARM:return glm::translate(I, { 0, -0.38f, 0 });
        case L_UPPER_LEG:return glm::translate(I, { 0.10f, -0.40f, 0 });
        case L_LOWER_LEG:return glm::translate(I, { 0, -0.38f, 0 });
        case R_UPPER_LEG:return glm::translate(I, { -0.10f, -0.40f, 0 });
        case R_LOWER_LEG:return glm::translate(I, { 0, -0.38f, 0 });
        default:return I;
        }
        };

    glm::mat4 gb[BONE_COUNT];
    gb[ROOT] = bl(ROOT);
    gb[SPINE] = gb[ROOT] * bl(SPINE);
    gb[HEAD] = gb[SPINE] * bl(HEAD);
    gb[L_UPPER_ARM] = gb[SPINE] * bl(L_UPPER_ARM);
    gb[L_FOREARM] = gb[L_UPPER_ARM] * bl(L_FOREARM);
    gb[R_UPPER_ARM] = gb[SPINE] * bl(R_UPPER_ARM);
    gb[R_FOREARM] = gb[R_UPPER_ARM] * bl(R_FOREARM);
    gb[L_UPPER_LEG] = gb[ROOT] * bl(L_UPPER_LEG);
    gb[L_LOWER_LEG] = gb[L_UPPER_LEG] * bl(L_LOWER_LEG);
    gb[R_UPPER_LEG] = gb[ROOT] * bl(R_UPPER_LEG);
    gb[R_LOWER_LEG] = gb[R_UPPER_LEG] * bl(R_LOWER_LEG);

    _skeleton.addBone("Root", -1, glm::inverse(gb[ROOT]));
    _skeleton.addBone("Spine", ROOT, glm::inverse(gb[SPINE]));
    _skeleton.addBone("Head", SPINE, glm::inverse(gb[HEAD]));
    _skeleton.addBone("L_UpperArm", SPINE, glm::inverse(gb[L_UPPER_ARM]));
    _skeleton.addBone("L_Forearm", L_UPPER_ARM, glm::inverse(gb[L_FOREARM]));
    _skeleton.addBone("R_UpperArm", SPINE, glm::inverse(gb[R_UPPER_ARM]));
    _skeleton.addBone("R_Forearm", R_UPPER_ARM, glm::inverse(gb[R_FOREARM]));
    _skeleton.addBone("L_UpperLeg", ROOT, glm::inverse(gb[L_UPPER_LEG]));
    _skeleton.addBone("L_LowerLeg", L_UPPER_LEG, glm::inverse(gb[L_LOWER_LEG]));
    _skeleton.addBone("R_UpperLeg", ROOT, glm::inverse(gb[R_UPPER_LEG]));
    _skeleton.addBone("R_LowerLeg", R_UPPER_LEG, glm::inverse(gb[R_LOWER_LEG]));

    for (int i = 0; i < BONE_COUNT; i++) {
        _skeleton.bone(i).localTransform = bl(i);
    }

    _skeleton.updateMatrices();
}

void HumanoidBuilder::buildBodyParts() {
    // Steve-style proportions (Minecraft: head=8, body=8x12x4, arms=4x12x4, legs=4x12x4 pixels)
    // Scaled to our world units (1 block = 1 unit)

    // Head: 0.48 x 0.50 x 0.48
    addBox(0.48f, 0.50f, 0.48f, HEAD, glm::vec3(0, 1.15f, 0));
    headRange = { (uint32_t)_indices.size() - 36, 36 };

    // Face features on HEAD bone (character faces -Z)
    // Eyes: dark boxes
    addBox(0.08f, 0.07f, 0.03f, HEAD, glm::vec3(-0.13f, 1.30f, 0.25f));
    addBox(0.08f, 0.07f, 0.03f, HEAD, glm::vec3(0.13f, 1.30f, 0.25f));
    faceEyes = { (uint32_t)_indices.size() - 72, 72 };

    // Nose
    addBox(0.08f, 0.08f, 0.06f, HEAD, glm::vec3(0, 1.18f, 0.28f));
    faceNose = { (uint32_t)_indices.size() - 36, 36 };

    // Mouth
    addBox(0.18f, 0.05f, 0.03f, HEAD, glm::vec3(0, 1.06f, 0.25f));
    faceMouth = { (uint32_t)_indices.size() - 36, 36 };

    // Hair: dark brown box slightly larger on top/back/sides of head
    addBox(0.52f, 0.18f, 0.52f, HEAD, glm::vec3(0, 1.44f, -0.02f));
    hair = { (uint32_t)_indices.size() - 36, 36 };

    // Body: 0.44 x 0.54 x 0.22
    addBox(0.44f, 0.54f, 0.22f, SPINE, glm::vec3(0, 0.47f, 0));
    bodyRange = { (uint32_t)_indices.size() - 36, 36 };

    // Shoulder joints (hidden inside body)
    addBox(0.14f, 0.14f, 0.14f, L_UPPER_ARM, glm::vec3(0.36f, 0.92f, 0));
    addBox(0.14f, 0.14f, 0.14f, R_UPPER_ARM, glm::vec3(-0.36f, 0.92f, 0));

    // Upper arms: 0.20 x 0.46 x 0.20
    addBox(0.20f, 0.46f, 0.20f, L_UPPER_ARM, glm::vec3(0.36f, 0.68f, 0));
    armL_upper = { (uint32_t)_indices.size() - 36, 36 };
    addBox(0.20f, 0.46f, 0.20f, R_UPPER_ARM, glm::vec3(-0.36f, 0.68f, 0));
    armR_upper = { (uint32_t)_indices.size() - 36, 36 };

    // Forearms: 0.18 x 0.44 x 0.18
    addBox(0.18f, 0.44f, 0.18f, L_FOREARM, glm::vec3(0.36f, 0.28f, 0));
    armL_lower = { (uint32_t)_indices.size() - 36, 36 };
    addBox(0.18f, 0.44f, 0.18f, R_FOREARM, glm::vec3(-0.36f, 0.28f, 0));
    armR_lower = { (uint32_t)_indices.size() - 36, 36 };

    // Upper legs: 0.22 x 0.46 x 0.22
    addBox(0.22f, 0.46f, 0.22f, L_UPPER_LEG, glm::vec3(0.14f, -0.27f, 0));
    legL_upper = { (uint32_t)_indices.size() - 36, 36 };
    addBox(0.22f, 0.46f, 0.22f, R_UPPER_LEG, glm::vec3(-0.14f, -0.27f, 0));
    legR_upper = { (uint32_t)_indices.size() - 36, 36 };

    // Lower legs: 0.20 x 0.46 x 0.20
    addBox(0.20f, 0.46f, 0.20f, L_LOWER_LEG, glm::vec3(0.14f, -0.70f, 0));
    legL_lower = { (uint32_t)_indices.size() - 36, 36 };
    addBox(0.20f, 0.46f, 0.20f, R_LOWER_LEG, glm::vec3(-0.14f, -0.70f, 0));
    legR_lower = { (uint32_t)_indices.size() - 36, 36 };
}
void HumanoidBuilder::addBox(float w, float h, float d, int bid, glm::vec3 pos) {
    auto v = makeBox(w, h, d, bid); for (auto& x : v)x.position += pos;
    uint32_t base = (uint32_t)_vertices.size(); auto idx = makeBoxIndices(base);
    for (auto& x : v)_vertices.push_back(x); for (auto& x : idx)_indices.push_back(x);
}

std::unique_ptr<SkinnedMesh> HumanoidBuilder::buildMesh() {
    return std::make_unique<SkinnedMesh>(_vertices, _indices);
}

static void btrs(int b, glm::vec3& p, glm::quat& r, glm::vec3& s) {
    r = glm::quat(1, 0, 0, 0); s = glm::vec3(1);
    switch (b) {
    case HumanoidBuilder::ROOT:p = { 0, 0, 0 }; break;
    case HumanoidBuilder::SPINE:p = { 0, 0.40f, 0 }; break;
    case HumanoidBuilder::HEAD:p = { 0, 0.55f, 0 }; break;
    case HumanoidBuilder::L_UPPER_ARM:p = { 0.33f, 0.45f, 0 }; break;
    case HumanoidBuilder::L_FOREARM:p = { 0, -0.38f, 0 }; break;
    case HumanoidBuilder::R_UPPER_ARM:p = { -0.33f, 0.45f, 0 }; break;
    case HumanoidBuilder::R_FOREARM:p = { 0, -0.38f, 0 }; break;
    case HumanoidBuilder::L_UPPER_LEG:p = { 0.10f, -0.40f, 0 }; break;
    case HumanoidBuilder::L_LOWER_LEG:p = { 0, -0.38f, 0 }; break;
    case HumanoidBuilder::R_UPPER_LEG:p = { -0.10f, -0.40f, 0 }; break;
    case HumanoidBuilder::R_LOWER_LEG:p = { 0, -0.38f, 0 }; break;
    default:p = { 0, 0, 0 }; break;
    }
}

void HumanoidBuilder::buildAnimations() {
    auto qI = glm::quat(1, 0, 0, 0);
    idleClip.name = "Idle"; idleClip.duration = 2.0f; idleClip.looping = true;
    waveClip.name = "Wave"; waveClip.duration = 1.5f; waveClip.looping = true;
    attackClip.name = "Attack"; attackClip.duration = 0.8f; attackClip.looping = false;
    walkClip.name = "Walk"; walkClip.duration = 1.0f; walkClip.looping = true;
    jumpClip.name = "Jump"; jumpClip.duration = 0.6f; jumpClip.looping = false;

    for (int b = 0; b < BONE_COUNT; b++) {
        // Idle
        {
            BoneTrack t; t.boneIndex = b; glm::vec3 bp; glm::quat br; glm::vec3 bs; btrs(b, bp, br, bs);
            if (b == ROOT) {
                t.keyframes.push_back({ 0, bp, br, bs });
                t.keyframes.push_back({ 1, bp + glm::vec3(0, 0.02f, 0), br, bs });
                t.keyframes.push_back({ 2, bp, br, bs });
            }
            else if (b == L_UPPER_ARM || b == R_UPPER_ARM) {
                float sign = (b == R_UPPER_ARM ? 1.f : -1.f);
                t.keyframes.push_back({ 0, bp, br, bs });
                t.keyframes.push_back({ 1, bp, glm::angleAxis(glm::radians(3.f * sign), glm::vec3(0, 0, 1)) * br, bs });
                t.keyframes.push_back({ 2, bp, br, bs });
            }
            else {
                t.keyframes.push_back({ 0, bp, br, bs });
            }

            idleClip.tracks.push_back(t);
        }

        // Wave
        {
            BoneTrack t; t.boneIndex = b; glm::vec3 bp; glm::quat br; glm::vec3 bs; btrs(b, bp, br, bs);
            if (b == R_UPPER_ARM) {
                auto r90 = glm::angleAxis(glm::radians(-90.f), glm::vec3(1, 0, 0)) * glm::angleAxis(glm::radians(15.f), glm::vec3(0, 1, 0));
                t.keyframes.push_back({ 0, bp, br, bs }); t.keyframes.push_back({ 0.4f, bp, r90, bs });
                t.keyframes.push_back({ 0.7f, bp, glm::angleAxis(glm::radians(-75.f), glm::vec3(1, 0, 0)) * glm::angleAxis(glm::radians(-10.f), glm::vec3(0, 1, 0)), bs });
                t.keyframes.push_back({ 1.1f, bp, glm::angleAxis(glm::radians(-100.f), glm::vec3(1, 0, 0)) * glm::angleAxis(glm::radians(25.f), glm::vec3(0, 1, 0)), bs });
                t.keyframes.push_back({ 1.5f, bp, r90, bs });
            }
            else {
                t.keyframes.push_back({ 0, bp, br, bs });
            }

            waveClip.tracks.push_back(t);
        }

        // Attack
        {
            BoneTrack t;
            t.boneIndex = b;

            glm::vec3 bp;
            glm::quat br;
            glm::vec3 bs;
            btrs(b, bp, br, bs);

            if (b == R_UPPER_ARM) {
                t.keyframes.push_back({ 0, bp, br, bs });
                t.keyframes.push_back({ 0.2f, bp, glm::angleAxis(glm::radians(25.f), glm::vec3(0, 1, 0)) * br, bs });
                t.keyframes.push_back({ 0.4f, bp, glm::angleAxis(glm::radians(-70.f), glm::vec3(0, 1, 0)) * glm::angleAxis(glm::radians(-20.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.8f, bp, br, bs });
            }
            else if (b == L_UPPER_ARM) {
                t.keyframes.push_back({ 0, bp, br, bs });
                t.keyframes.push_back({ 0.2f, bp, glm::angleAxis(glm::radians(-25.f), glm::vec3(0, 1, 0)) * br, bs });
                t.keyframes.push_back({ 0.4f, bp, glm::angleAxis(glm::radians(70.f), glm::vec3(0, 1, 0)) * glm::angleAxis(glm::radians(-20.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.8f, bp, br, bs });
            }
            else if (b == ROOT) {
                t.keyframes.push_back({ 0, bp, br, bs });
                t.keyframes.push_back({ 0.4f, bp + glm::vec3(0, 0, 0.06f), br, bs });
                t.keyframes.push_back({ 0.8f, bp, br, bs });
            }
            else t.keyframes.push_back({ 0, bp, br, bs });
            attackClip.tracks.push_back(t);
        }

        // Walk — natural gait cycle (1.0s, 8 keyframes for smooth motion)
        {
            BoneTrack t;
            t.boneIndex = b;

            glm::vec3 bp;
            glm::quat br;
            glm::vec3 bs;
            btrs(b, bp, br, bs);

            if (b == L_UPPER_LEG) {
                // Forward swing (0→0.5), back swing (0.5→1.0), with knee bend at passing
                t.keyframes.push_back({ 0.000f, bp, glm::angleAxis(glm::radians(28.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.125f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.250f, bp, br, bs });
                t.keyframes.push_back({ 0.375f, bp, glm::angleAxis(glm::radians(-15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.500f, bp, glm::angleAxis(glm::radians(-28.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.625f, bp, glm::angleAxis(glm::radians(-15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.750f, bp, br, bs });
                t.keyframes.push_back({ 0.875f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 1.000f, bp, glm::angleAxis(glm::radians(28.f), glm::vec3(1, 0, 0)) * br, bs });
            }
            else if (b == R_UPPER_LEG) {
                // Opposite phase to left leg
                t.keyframes.push_back({ 0.000f, bp, glm::angleAxis(glm::radians(-28.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.125f, bp, glm::angleAxis(glm::radians(-15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.250f, bp, br, bs });
                t.keyframes.push_back({ 0.375f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.500f, bp, glm::angleAxis(glm::radians(28.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.625f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.750f, bp, br, bs });
                t.keyframes.push_back({ 0.875f, bp, glm::angleAxis(glm::radians(-15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 1.000f, bp, glm::angleAxis(glm::radians(-28.f), glm::vec3(1, 0, 0)) * br, bs });
            }
            else if (b == L_LOWER_LEG) {
                // Knee bend at foot-lift moments
                t.keyframes.push_back({ 0.000f, bp, br, bs });
                t.keyframes.push_back({ 0.250f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.500f, bp, br, bs });
                t.keyframes.push_back({ 0.750f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 1.000f, bp, br, bs });
            }
            else if (b == R_LOWER_LEG) {
                t.keyframes.push_back({ 0.000f, bp, br, bs });
                t.keyframes.push_back({ 0.250f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.500f, bp, br, bs });
                t.keyframes.push_back({ 0.750f, bp, glm::angleAxis(glm::radians(15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 1.000f, bp, br, bs });
            }
            else if (b == L_UPPER_ARM) {
                // Arms swing opposite to legs
                t.keyframes.push_back({ 0.000f, bp, glm::angleAxis(glm::radians(-18.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.250f, bp, br, bs });
                t.keyframes.push_back({ 0.500f, bp, glm::angleAxis(glm::radians(18.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.750f, bp, br, bs });
                t.keyframes.push_back({ 1.000f, bp, glm::angleAxis(glm::radians(-18.f), glm::vec3(1, 0, 0)) * br, bs });
            }
            else if (b == R_UPPER_ARM) {
                t.keyframes.push_back({ 0.000f, bp, glm::angleAxis(glm::radians(18.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.250f, bp, br, bs });
                t.keyframes.push_back({ 0.500f, bp, glm::angleAxis(glm::radians(-18.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.750f, bp, br, bs });
                t.keyframes.push_back({ 1.000f, bp, glm::angleAxis(glm::radians(18.f), glm::vec3(1, 0, 0)) * br, bs });
            }
            else if (b == ROOT) {
                // Body bob: lowest at double-support, highest at passing
                t.keyframes.push_back({ 0.000f, bp + glm::vec3(0, -0.05f, 0), br, bs });
                t.keyframes.push_back({ 0.125f, bp + glm::vec3(0, 0.00f, 0), br, bs });
                t.keyframes.push_back({ 0.250f, bp + glm::vec3(0, 0.03f, 0), br, bs });
                t.keyframes.push_back({ 0.375f, bp + glm::vec3(0, 0.00f, 0), br, bs });
                t.keyframes.push_back({ 0.500f, bp + glm::vec3(0, -0.05f, 0), br, bs });
                t.keyframes.push_back({ 0.625f, bp + glm::vec3(0, 0.00f, 0), br, bs });
                t.keyframes.push_back({ 0.750f, bp + glm::vec3(0, 0.03f, 0), br, bs });
                t.keyframes.push_back({ 0.875f, bp + glm::vec3(0, 0.00f, 0), br, bs });
                t.keyframes.push_back({ 1.000f, bp + glm::vec3(0, -0.05f, 0), br, bs });
            }
            else if (b == SPINE) {
                // Slight side-to-side sway
                t.keyframes.push_back({ 0.000f, bp, glm::angleAxis(glm::radians(2.f), glm::vec3(0, 0, 1)) * br, bs });
                t.keyframes.push_back({ 0.500f, bp, glm::angleAxis(glm::radians(-2.f), glm::vec3(0, 0, 1)) * br, bs });
                t.keyframes.push_back({ 1.000f, bp, glm::angleAxis(glm::radians(2.f), glm::vec3(0, 0, 1)) * br, bs });
            }
            else {
                t.keyframes.push_back({ 0.0f, bp, br, bs });
            }

            walkClip.tracks.push_back(t);
        }

        // Jump
        {
            BoneTrack t; t.boneIndex = b; glm::vec3 bp; glm::quat br; glm::vec3 bs; btrs(b, bp, br, bs);
            if (b == ROOT) {
                t.keyframes.push_back({ 0.00f, bp, br, bs });
                t.keyframes.push_back({ 0.15f, bp + glm::vec3(0, -0.12f, 0), br, bs });
                t.keyframes.push_back({ 0.25f, bp + glm::vec3(0, 0.25f, 0), br, bs });
                t.keyframes.push_back({ 0.45f, bp + glm::vec3(0, 0.15f, 0), br, bs });
                t.keyframes.push_back({ 0.60f, bp, br, bs });
            }
            else if (b == L_UPPER_LEG || b == R_UPPER_LEG) {
                t.keyframes.push_back({ 0.00f, bp, br, bs });
                t.keyframes.push_back({ 0.15f, bp, glm::angleAxis(glm::radians(20.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.25f, bp, glm::angleAxis(glm::radians(-15.f), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.45f, bp, br, bs });
                t.keyframes.push_back({ 0.60f, bp, br, bs });
            }
            else if (b == L_UPPER_ARM || b == R_UPPER_ARM) {
                float sign = (b == R_UPPER_ARM ? 1.f : -1.f);
                t.keyframes.push_back({ 0.00f, bp, br, bs });
                t.keyframes.push_back({ 0.15f, bp, glm::angleAxis(glm::radians(-20.f * sign), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.25f, bp, glm::angleAxis(glm::radians(40.f * sign), glm::vec3(1, 0, 0)) * br, bs });
                t.keyframes.push_back({ 0.60f, bp, br, bs });
            }
            else {
                t.keyframes.push_back({ 0.0f, bp, br, bs });
            }

            jumpClip.tracks.push_back(t);
        }
    }
}

// ============================================================
//  PRIMITIVE: Box
// ============================================================
static void setBone(SkinnedVertex& sv, int id) {
    sv.boneIds[0] = id;
    sv.boneIds[1] = sv.boneIds[2] = sv.boneIds[3] = 0;
    sv.weights[0] = 1;
    sv.weights[1] = sv.weights[2] = sv.weights[3] = 0;
}

std::vector<SkinnedVertex>HumanoidBuilder::makeBox(float w, float h, float d, int bid) {
    float hw = w * 0.5f, hh = h * 0.5f, hd = d * 0.5f;
    std::vector<SkinnedVertex> v;
    auto add = [&](glm::vec3 p, glm::vec3 n) {
        SkinnedVertex sv;
        sv.position = p; sv.normal = n;
        setBone(sv, bid);
        v.push_back(sv);
        };

    add({ -hw, +hh, -hd }, { +0, +1, +0 });
    add({ +hw, +hh, -hd }, { +0, +1, +0 });
    add({ +hw, +hh, +hd }, { +0, +1, +0 });
    add({ -hw, +hh, +hd }, { +0, +1, +0 });
    add({ -hw, -hh, +hd }, { +0, -1, +0 });
    add({ +hw, -hh, +hd }, { +0, -1, +0 });
    add({ +hw, -hh, -hd }, { +0, -1, +0 });
    add({ -hw, -hh, -hd }, { +0, -1, +0 });
    add({ +hw, +hh, +hd }, { +1, +0, +0 });
    add({ +hw, +hh, -hd }, { +1, +0, +0 });
    add({ +hw, -hh, -hd }, { +1, +0, +0 });
    add({ +hw, -hh, +hd }, { +1, +0, +0 });
    add({ -hw, +hh, -hd }, { -1, +0, +0 });
    add({ -hw, +hh, +hd }, { -1, +0, +0 });
    add({ -hw, -hh, +hd }, { -1, +0, +0 });
    add({ -hw, -hh, -hd }, { -1, +0, +0 });
    add({ -hw, +hh, +hd }, { +0, +0, +1 });
    add({ +hw, +hh, +hd }, { +0, +0, +1 });
    add({ +hw, -hh, +hd }, { +0, +0, +1 });
    add({ -hw, -hh, +hd }, { +0, +0, +1 });
    add({ +hw, +hh, -hd }, { +0, +0, -1 });
    add({ -hw, +hh, -hd }, { +0, +0, -1 });
    add({ -hw, -hh, -hd }, { +0, +0, -1 });
    add({ +hw, -hh, -hd }, { +0, +0, -1 });

    return v;
}
std::vector<uint32_t>HumanoidBuilder::makeBoxIndices(uint32_t base) {
    std::vector<uint32_t>idx;
    for (int f = 0; f < 6; f++) {
        uint32_t b = base + f * 4;
        idx.insert(idx.end(), { b, b + 1, b + 2, b, b + 2, b + 3 });
    }

    return idx;
}
