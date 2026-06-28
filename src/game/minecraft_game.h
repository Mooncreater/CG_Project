#pragma once
#include <functional>

#include "../base/application.h"
#include "../base/skybox.h"
#include "../animation/skeleton.h"
#include "../animation/animation_clip.h"
#include "../animation/animator.h"
#include "../animation/skinned_mesh.h"
#include "../animation/skinned_shader.h"
#include "../animation/humanoid.h"
#include "../mesh/mesh.h"
#include "../primitives/primitives.h"
#include "../render_util/simple_lit.h"
#include "../render_util/textured_lit.h"
#include "../camera/free_camera.h"
#include "../orbit_control/orbit_control.h"
#include "../collision/aabb.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>

// ============================================================
// Block types
// ============================================================
enum BlockType : int {
    BT_AIR = 0,
    BT_GRASS,
    BT_DIRT,
    BT_STONE,
    BT_WOOD,
    BT_LEAVES,
    BT_SAND,
    BT_WATER,
    BT_COUNT
};

inline const char* blockTypeName(int t) {
    switch (t) {
        case BT_GRASS:  return "Grass";
        case BT_DIRT:   return "Dirt";
        case BT_STONE:  return "Stone";
        case BT_WOOD:   return "Wood";
        case BT_LEAVES: return "Leaves";
        case BT_SAND:   return "Sand";
        case BT_WATER:  return "Water";
        default:        return "???";
    }
}

inline glm::vec3 blockTypeColor(int t) {
    switch (t) {
        case BT_GRASS:  return {0.35f, 0.65f, 0.25f};
        case BT_DIRT:   return {0.55f, 0.40f, 0.25f};
        case BT_STONE:  return {0.50f, 0.50f, 0.50f};
        case BT_WOOD:   return {0.55f, 0.40f, 0.20f};
        case BT_LEAVES: return {0.20f, 0.55f, 0.15f};
        case BT_SAND:   return {0.85f, 0.80f, 0.55f};
        case BT_WATER:  return {0.25f, 0.45f, 0.85f};
        default:        return {0.50f, 0.50f, 0.50f};
    }
}

struct ivec3_hash {
    size_t operator()(const glm::ivec3& v) const {
        return (size_t)(v.x * 73856093 ^ v.y * 19349663 ^ v.z * 83492791);
    }
};

// ============================================================
// Dropped item (floating block after destruction)
// ============================================================
struct DroppedItem {
    glm::vec3 pos;
    glm::vec3 pickupStartPos; // position when pickup started
    int blockType;
    float lifeTime = 0;
    float bobPhase = 0;
    bool beingPickedUp = false;
    float pickupTimer = 0;
};

class MinecraftGame : public Application {
public:
    MinecraftGame(const Options& options);
    ~MinecraftGame();

private:
    // ===== Animation / Character =====
    HumanoidBuilder _builder;
    std::unique_ptr<SkinnedMesh> _mesh;
    SkinnedShader _shader;
    Animator _animator{_builder.skeleton()};

    // ===== World =====
    std::unique_ptr<SkyBox> _skybox;
    std::unique_ptr<Mesh> _cubeMesh;
    SimpleLitShader _litShader;
    TexturedLitShader _texLitShader;
    std::unique_ptr<Mesh> _earthMesh;
    GLuint _earthTex = 0;
    std::vector<std::unique_ptr<Mesh>> _primitiveMeshes;
    std::vector<std::string> _primitiveNames;
    InstancedTexturedLitShader _instTexLitShader;
    GLuint _blockAtlas = 0;
    static constexpr int ATLAS_SIZE = 1024;
    static constexpr int ATLAS_COLS = 8;
    static constexpr int TILE_PX = 128;

    std::unordered_map<glm::ivec3, int, ivec3_hash> _blocks;  // pos → BlockType
    std::vector<DroppedItem> _droppedItems;

    // Instanced rendering
    GLuint _instancedCubeVAO = 0;
    GLuint _instanceVBO = 0;

    // ===== Camera =====
    FreeCamera _fc;
    OrbitControl _ctrl{_fc};

    // ===== Player =====
    glm::vec3 _playerPos{0, 0.5f, 0};
    float _playerYaw = 0;
    static constexpr float PLAYER_HALF_W = 0.25f;
    static constexpr float PLAYER_HALF_H = 1.0f;
    static constexpr float PICKUP_RANGE = 2.5f;
    float _playerYVel = 0;
    bool _onGround = false;
    bool _jumping = false;
    float _jumpTimer = 0;

    enum AnimState { IDLE, WALK, JUMP };
    AnimState _animState = IDLE;

    // ===== Blocks / Inventory =====
    int _selectedBlock = BT_GRASS;
    bool _inventoryOpen = false;
    bool _settingsOpen = false;

    // ===== Ghost block preview =====
    glm::ivec3 _ghostHit{0};
    glm::ivec3 _ghostPlace{0};
    bool _ghostHitValid = false;
    bool _ghostPlaceValid = false;

    // ===== Lighting =====
    glm::vec3 _sunDir{0.5f, 1, 0.3f};
    glm::vec3 _sunColor{1, 0.95f, 0.85f};
    glm::vec3 _ptPos{3, 4, -2};
    glm::vec3 _ptColor{0.3f, 0.5f, 1};
    float _ptIntensity = 5;
    float _ambient = 0.3f;
    float _shininess = 32.0f;
    glm::vec3 _specColor{1, 1, 1};
    float _ptAttenConst = 1.0f;
    float _ptAttenLinear = 0.09f;
    float _ptAttenQuad = 0.032f;

    // ===== Shadow mapping =====
    GLuint _shadowFBO = 0;
    GLuint _shadowMap = 0;
    static constexpr int SHADOW_RES = 2048;
    glm::mat4 _lightSpaceMatrix{1};
    float _shadowBias = 0.0005f;
    bool _shadowsOn = true;
    std::unique_ptr<GLSLProgram> _shadowShader;
    void initShadowMap();
    void renderShadowPass();
    PhongParams makePhongParams(const glm::vec3& camPos) const;

    // ===== Mouse tracking =====
    bool _rightWasDown = false;
    float _rightStartMX = 0, _rightStartMY = 0;
    float _gameTime = 0;
    float _screenshotFlash = 0;
    float _preZoomDist = 5; float _preZoomYaw = 0; float _preZoomPitch = -30;
    glm::vec3 _preZoomTarget{0, 3, 0};
    bool _zoomedOut = false;

    // ===== First/third person =====
    bool _firstPerson = false;
    float _fpPitch = 0;
    float _prevMX = 0, _prevMY = 0;
    float _smoothFPS = 60.0f;

    // ===== World generation =====
    static constexpr int WORLD_SIZE = 32;
    int _worldSeed = 42;

    float noise2D(float x, float y) const;
    float getTerrainHeight(float wx, float wz) const;
    void initWorld();
    void generateTerrain();
    void placeTree(int bx, int by, int bz);

    // ===== Texture atlas =====
    void buildTextureAtlas();
    void buildEarthTexture();
    void createShowcaseObjects();

    // ===== Block queries =====
    int getBlock(int x, int y, int z) const;
    bool isSolidBlock(int x, int y, int z) const;

    glm::vec3 fpForward() const;

    void handleInput() override;
    void renderFrame() override;

    void handleInputFirstPerson();
    void handleInputThirdPerson();

    void computeGhost();
    void drawGhost(const glm::mat4& proj, const glm::mat4& view);
    bool resolvePlayerMovement(glm::vec3& pos, const glm::vec3& desired);

    // ===== Dropped items =====
    void spawnDroppedItem(const glm::ivec3& blockPos, int bt);
    void updateDroppedItems();
    void drawDroppedItems(const glm::mat4& proj, const glm::mat4& view);

    void drawBlocks(const glm::mat4& proj, const glm::mat4& view);
    void drawCharacter(const glm::mat4& proj, const glm::mat4& view);
    void drawUI();
    void drawInventory();

    void setLight();
};
