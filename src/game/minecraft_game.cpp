#include "minecraft_game.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdio>
#include <algorithm>
#include <direct.h>
#include <stb_image_write.h>
#include <stb_image.h>
#include <functional>
// Simple hash-based 2D noise
// ============================================================
// ============================================================
static uint32_t hash(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

float MinecraftGame::noise2D(float x, float y) const {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - ix, fy = y - iy;
    // smoothstep
    float u = fx * fx * (3 - 2 * fx);
    float v = fy * fy * (3 - 2 * fy);
    uint32_t s = (uint32_t)(_worldSeed * 12345 + ix * 1103515245 + iy * 12345);
    float a = (hash(s + 0) % 10000) / 10000.0f;
    float b = (hash(s + 1) % 10000) / 10000.0f;
    float c = (hash(s + 2) % 10000) / 10000.0f;
    float d = (hash(s + 3) % 10000) / 10000.0f;
    return (a * (1 - u) + b * u) * (1 - v) + (c * (1 - u) + d * u) * v;
}

float MinecraftGame::getTerrainHeight(float wx, float wz) const {
    float h = 0;
    h += noise2D(wx * 0.02f, wz * 0.02f) * 12.0f;
    h += noise2D(wx * 0.05f, wz * 0.05f) * 5.0f;
    h += noise2D(wx * 0.12f, wz * 0.12f) * 2.0f;
    return h;
}


// ============================================================
// Procedural skybox generator — mountain + water scenery
// ============================================================
// Procedural skybox — mountain + lake scenery
// ============================================================
static void generateMountainSkybox(const std::string& dir) {
    const int SZ = 1024;
    std::vector<uint8_t> pixels(SZ * SZ * 3);

    auto rhash = [](int x, int y, int seed) -> float {
        uint32_t h = (uint32_t)(x * 374761393 + y * 668265263 + seed * 1274126177);
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = ((h >> 16) ^ h) * 0x45d9f3b;
        h = (h >> 16) ^ h;
        return (float)(h % 10000) / 10000.0f;
    };

    auto snoise = [&](float x, float y, int seed) -> float {
        int ix = (int)floorf(x), iy = (int)floorf(y);
        float fx = x - ix, fy = y - iy;
        float u = fx * fx * (3 - 2 * fx);
        float v = fy * fy * (3 - 2 * fy);
        float a = rhash(ix, iy, seed), b = rhash(ix + 1, iy, seed);
        float c = rhash(ix, iy + 1, seed), d = rhash(ix + 1, iy + 1, seed);
        return (a * (1 - u) + b * u) * (1 - v) + (c * (1 - u) + d * u) * v;
    };

    // Multi-octave mountain height
    auto mtnH = [&](float u, int seed) -> float {
        float h = snoise(u * 2.5f, 0.3f, seed) * 0.55f;
        h += snoise(u * 6.0f, 0.5f, seed + 100) * 0.30f;
        h += snoise(u * 13.0f, 0.7f, seed + 200) * 0.12f;
        h += snoise(u * 25.0f, 0.4f, seed + 300) * 0.05f;
        return h * 0.6f + 0.12f;
    };

    const char* fnames[6] = {"Right_Tex","Left_Tex","Up_Tex","Down_Tex","Front_Tex","Back_Tex"};

    for (int face = 0; face < 6; face++) {
        for (int y = 0; y < SZ; y++) {
            for (int x = 0; x < SZ; x++) {
                float u = (float)x / SZ;
                float v = 1.0f - (float)y / SZ;
                uint8_t r = 0, g = 0, b = 0;

                if (face == 2) {
                    // UP: Deep blue sky + scattered clouds
                    float skyB = 0.35f + v * 0.45f;
                    float cloud = snoise(u*7.0f, v*5.0f, 500)*0.5f + snoise(u*3.5f, v*2.5f, 600)*0.35f;
                    float cld = cloud > 0.55f ? (cloud-0.55f)/0.45f : 0.0f;
                    r = (uint8_t)(70 + (int)(skyB*120 + cld*65));
                    g = (uint8_t)(120 + (int)(skyB*100 + cld*35));
                    b = (uint8_t)(180 + (int)(skyB*75));
                }
                else if (face == 3) {
                    // DOWN: Dark lake bottom
                    float ripple = snoise(u*14.0f, v*14.0f, 700)*0.08f;
                    r = (uint8_t)(8 + (int)(ripple*20));
                    g = (uint8_t)(18 + (int)(ripple*30));
                    b = (uint8_t)(45 + (int)(ripple*40));
                }
                else {
                    // HORIZONTAL faces
                    float sunGlow = 0;
                    if (face == 0) sunGlow = expf(-((u-0.75f)*(u-0.75f)+(v-0.55f)*(v-0.55f))*8.0f)*0.3f;

                    float horizon = 0.08f;
                    float mtBase = 0.05f;
                    float mh = mtnH(u, face*1000+42);
                    float mtTop = mtBase + mh;

                    if (v > horizon + 0.03f) {
                        // === SKY ===
                        float t = (v - horizon - 0.03f) / (0.97f - horizon);
                        float skyB = 0.25f + t * 0.45f;
                        float haze = (1.0f - t) * (1.0f - t) * 0.15f;
                        float cloud = snoise(u*5.0f, v*3.5f, 800)*0.35f + snoise(u*2.5f, v*1.8f, 900)*0.25f;
                        r = (uint8_t)(60 + (int)((skyB + cloud*0.5f) * 170 + sunGlow*255));
                        g = (uint8_t)(110 + (int)((skyB + cloud*0.4f + haze) * 120 + sunGlow*200));
                        b = (uint8_t)(170 + (int)((skyB + cloud*0.3f + haze) * 85));
                    }
                    else if (v > mtTop) {
                        // === SKY BELOW HORIZON (distant atmosphere) ===
                        float t = (v - mtTop) / (horizon + 0.03f - mtTop + 0.001f);
                        float haze = (1.0f - t) * 0.4f;
                        r = (uint8_t)(140 + (int)(haze*60 + sunGlow*200));
                        g = (uint8_t)(150 + (int)(haze*60 + sunGlow*100));
                        b = (uint8_t)(160 + (int)(haze*80));
                    }
                    else if (v > mtBase) {
                        // === MOUNTAIN ===
                        float t = (v - mtBase) / (mtTop - mtBase + 0.001f);
                        if (t > 0.78f) {
                            // Snow cap
                            float snow = snoise(u*12.0f, t*8.0f, 42)*0.25f;
                            r = (uint8_t)(225 + (int)(snow*30));
                            g = (uint8_t)(230 + (int)(snow*25));
                            b = (uint8_t)(235 + (int)(snow*20));
                        }
                        else if (t > 0.4f) {
                            // Rocky mid-section
                            float rock = snoise(u*10.0f, t*7.0f, 45)*0.35f;
                            r = (uint8_t)(55 + (int)(rock*45));
                            g = (uint8_t)(50 + (int)(rock*38));
                            b = (uint8_t)(40 + (int)(rock*30));
                        }
                        else {
                            // Green vegetation base
                            float veg = snoise(u*9.0f, t*6.0f, 48)*0.25f;
                            r = (uint8_t)(30 + (int)(veg*20));
                            g = (uint8_t)(60 + (int)(veg*40));
                            b = (uint8_t)(20 + (int)(veg*15));
                        }
                    }
                    else {
                        // === WATER ===
                        float ripple = snoise(u*18.0f, v*22.0f, 701)*0.10f
                                     + snoise(u*10.0f, v*10.0f, 702)*0.06f;
                        float depth = 1.0f - (mtBase - v) * 1.5f;
                        if (depth < 0.2f) depth = 0.2f;
                        // Reflect sky color in water
                        float refl = 0.5f;
                        r = (uint8_t)((20 + ripple*40)*depth + (70*sunGlow)*refl);
                        g = (uint8_t)((45 + ripple*70)*depth + (120*sunGlow)*refl);
                        b = (uint8_t)((100 + ripple*100)*depth + (170*sunGlow)*refl);
                    }
                }

                r = (uint8_t)std::min(255, std::max(0, (int)r));
                g = (uint8_t)std::min(255, std::max(0, (int)g));
                b = (uint8_t)std::min(255, std::max(0, (int)b));
                int idx = (y * SZ + x) * 3;
                pixels[idx+0] = r; pixels[idx+1] = g; pixels[idx+2] = b;
            }
        }
        std::string path = dir + "/" + fnames[face] + ".jpg";
        stbi_write_jpg(path.c_str(), SZ, SZ, 3, pixels.data(), 90);
    }
}

// ============================================================
// Constructor / Destructor
// ============================================================
MinecraftGame::MinecraftGame(const Options& o) : Application(o), _worldSeed(42) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    _mesh = _builder.buildMesh();
    _cubeMesh = std::make_unique<Mesh>(
        Primitives::CreateCube(1.0f).vertices,
        Primitives::CreateCube(1.0f).indices);
    // Setup instanced VAO (cube mesh + instance position buffer)
    {
        auto cubeData = Primitives::CreateCube(1.0f);
        glGenVertexArrays(1, &_instancedCubeVAO);
        glBindVertexArray(_instancedCubeVAO);

        // Position buffer
        GLuint posVBO, normVBO, uvVBO, ibo;
        glGenBuffers(1, &posVBO);
        glBindBuffer(GL_ARRAY_BUFFER, posVBO);
        std::vector<float> pbuf; for (auto& v : cubeData.vertices) { pbuf.push_back(v.position.x); pbuf.push_back(v.position.y); pbuf.push_back(v.position.z); }
        glBufferData(GL_ARRAY_BUFFER, pbuf.size() * sizeof(float), pbuf.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &normVBO);
        glBindBuffer(GL_ARRAY_BUFFER, normVBO);
        std::vector<float> nbuf; for (auto& v : cubeData.vertices) { nbuf.push_back(v.normal.x); nbuf.push_back(v.normal.y); nbuf.push_back(v.normal.z); }
        glBufferData(GL_ARRAY_BUFFER, nbuf.size() * sizeof(float), nbuf.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

        glGenBuffers(1, &uvVBO);
        glBindBuffer(GL_ARRAY_BUFFER, uvVBO);
        std::vector<float> ubuf; for (auto& v : cubeData.vertices) { ubuf.push_back(v.texCoord.x); ubuf.push_back(v.texCoord.y); }
        glBufferData(GL_ARRAY_BUFFER, ubuf.size() * sizeof(float), ubuf.data(), GL_STATIC_DRAW);
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

        // Index buffer
        glGenBuffers(1, &ibo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeData.indices.size() * sizeof(uint32_t), cubeData.indices.data(), GL_STATIC_DRAW);

        // Instance VBO
        glGenBuffers(1, &_instanceVBO);
        glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
        glVertexAttribDivisor(3, 1);

        glBindVertexArray(0);
    }

    buildTextureAtlas();
    buildEarthTexture();
    createShowcaseObjects();

    // Generate mountain-water skybox textures
    std::string skyDir = o.assetRootDir + "/texture/skybox_mountain/";
    #ifdef _WIN32
    _mkdir(skyDir.c_str());
    #else
    mkdir(skyDir.c_str(), 0755);
    #endif
    generateMountainSkybox(skyDir);

    _skybox = std::make_unique<SkyBox>(std::vector<std::string>{
        skyDir + "Right_Tex.jpg", skyDir + "Left_Tex.jpg",
        skyDir + "Up_Tex.jpg",    skyDir + "Down_Tex.jpg",
        skyDir + "Front_Tex.jpg", skyDir + "Back_Tex.jpg"
    });
    _animator.play(&_builder.idleClip);

    initWorld();

    float spawnY = 4.5f;
    _playerPos = glm::vec3(0, spawnY, 0);

    _fc.target = _playerPos + glm::vec3(0, 1.0f, 0);
    _fc.dist = 5;
    _fc.pitch = 25;

    initShadowMap();
}

MinecraftGame::~MinecraftGame() {
    if (_shadowFBO) glDeleteFramebuffers(1, &_shadowFBO);
    if (_shadowMap) glDeleteTextures(1, &_shadowMap);
    if (_blockAtlas) glDeleteTextures(1, &_blockAtlas);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

// ============================================================
// World init — procedural terrain
// ============================================================
void MinecraftGame::initWorld() {
    generateTerrain();
}

void MinecraftGame::generateTerrain() {
    const int SZ = WORLD_SIZE;
    const int CLEAR_RADIUS = 14;
    const int TREE_CLEAR = 18;

    for (int x = -SZ; x <= SZ; ++x) {
        for (int z = -SZ; z <= SZ; ++z) {
            float dist = sqrtf((float)(x * x + z * z));
            float hf = getTerrainHeight((float)x, (float)z);
            int groundY;
            float t = (dist - CLEAR_RADIUS + 4.0f) / 4.0f;
            if (t < 0) t = 0; if (t > 1) t = 1;
            groundY = (int)roundf(3.0f * (1 - t) + roundf(hf) * t);

            for (int y = -4; y <= groundY - 4; ++y)
                _blocks[{x, y, z}] = BT_STONE;
            for (int y = groundY - 3; y <= groundY - 1; ++y)
                _blocks[{x, y, z}] = BT_DIRT;
            _blocks[{x, groundY, z}] = BT_GRASS;
            if (groundY <= 2 && dist > CLEAR_RADIUS) {
                _blocks[{x, groundY, z}] = BT_SAND;
                for (int y = groundY - 2; y < groundY; ++y)
                    _blocks[{x, y, z}] = BT_SAND;
            }
            if (groundY <= 0) {
                for (int y = 1; y >= groundY + 1; --y)
                    _blocks[{x, y, z}] = BT_WATER;
                _blocks[{x, 1, z}] = BT_WATER;
            }
        }
    }

    for (int x = -SZ + 1; x < SZ; ++x) {
        for (int z = -SZ + 1; z < SZ; ++z) {
            float dist = sqrtf((float)(x * x + z * z));
            if (dist < TREE_CLEAR) continue;
            if ((hash((uint32_t)(x * 1931 + z * 8327 + _worldSeed * 999)) % 100) < 8) {
                int groundY = (int)roundf(getTerrainHeight((float)x, (float)z));
                auto git = _blocks.find({x, groundY, z});
                if (git != _blocks.end() && git->second == BT_GRASS) {
                    bool blocked = false;
                    for (int dx = -2; dx <= 2 && !blocked; ++dx)
                        for (int dz = -2; dz <= 2 && !blocked; ++dz)
                            if (_blocks.find({x + dx, groundY + 1, z + dz}) != _blocks.end())
                                blocked = true;
                    if (!blocked) placeTree(x, groundY + 1, z);
                }
            }
        }
    }
}

void MinecraftGame::placeTree(int bx, int by, int bz) {
    // Trunk: 4-5 blocks tall
    int trunkH = 4 + (int)(hash((uint32_t)(bx * 777 + bz * 333)) % 2);
    for (int y = 0; y < trunkH; ++y)
        _blocks[{bx, by + y, bz}] = BT_WOOD;

    // Leaves: 3-layer canopy
    int leafBase = by + trunkH - 1;
    for (int dy = 0; dy <= 2; ++dy) {
        int r = (dy == 1) ? 2 : 1;
        for (int dx = -r; dx <= r; ++dx)
            for (int dz = -r; dz <= r; ++dz) {
                if (dx == 0 && dz == 0 && dy < 2) continue; // skip trunk
                int lx = bx + dx, ly = leafBase + dy, lz = bz + dz;
                auto it = _blocks.find({lx, ly, lz});
                if (it == _blocks.end())
                    _blocks[{lx, ly, lz}] = BT_LEAVES;
            }
    }
    // top leaf
    _blocks[{bx, leafBase + 3, bz}] = BT_LEAVES;
}

// ============================================================
// Shadow Mapping
// ============================================================
void MinecraftGame::initShadowMap() {
    glGenFramebuffers(1, &_shadowFBO);
    glGenTextures(1, &_shadowMap);
    glBindTexture(GL_TEXTURE_2D, _shadowMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        SHADOW_RES, SHADOW_RES, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1, 1, 1, 1};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    // NO compare mode — we do manual comparison in shader

    glBindFramebuffer(GL_FRAMEBUFFER, _shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _shadowMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Simple depth-only shader
    const char* shadowVS = R"(#version 330 core
        layout(location=0) in vec3 aP;
        uniform mat4 uLS, uM;
        void main(){
            gl_Position=uLS*uM*vec4(aP,1);
        })";
    const char* shadowFS = R"(#version 330 core
        void main(){})";
    _shadowShader = std::make_unique<GLSLProgram>();
    _shadowShader->attachVertexShader(shadowVS);
    _shadowShader->attachFragmentShader(shadowFS);
    _shadowShader->link();
}

void MinecraftGame::renderShadowPass() {
    if (!_shadowsOn) return;
    // Compute light-space matrix
    glm::vec3 sunDir = glm::normalize(-_sunDir);
    float range = 30.0f;
    glm::vec3 lightPos = _playerPos - sunDir * range * 0.5f;

    glm::vec3 up = glm::vec3(0, 1, 0);
    if (fabsf(glm::dot(glm::normalize(sunDir), up)) > 0.99f)
        up = glm::vec3(1, 0, 0);

    auto lightView = glm::lookAt(lightPos, lightPos + sunDir, up);
    auto lightProj = glm::ortho(-range, range, -range, range, 0.1f, range * 2.0f);
    _lightSpaceMatrix = lightProj * lightView;

    glViewport(0, 0, SHADOW_RES, SHADOW_RES);
    glBindFramebuffer(GL_FRAMEBUFFER, _shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glDisable(GL_BLEND);

    // Collect opaque instance positions
    static std::vector<glm::vec3> shadowPos;
    shadowPos.clear();
    for (auto& kv : _blocks) {
        if (kv.second == BT_AIR || kv.second == BT_WATER || kv.second == BT_LEAVES) continue;
        shadowPos.push_back(glm::vec3(kv.first));
    }
    if (shadowPos.empty()) { glCullFace(GL_BACK); glBindFramebuffer(GL_FRAMEBUFFER, 0); return; }

    glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, shadowPos.size() * sizeof(glm::vec3),
                 shadowPos.data(), GL_DYNAMIC_DRAW);

    // Simple instanced shadow shader
    static std::unique_ptr<GLSLProgram> instShadowProg;
    if (!instShadowProg) {
        const char* vs = R"(#version 330 core
            layout(location=0) in vec3 aP; layout(location=3) in vec3 aInstP;
            uniform mat4 uLS;
            void main(){ gl_Position=uLS*vec4(aP+aInstP,1); })";
        instShadowProg = std::make_unique<GLSLProgram>();
        instShadowProg->attachVertexShader(vs);
        instShadowProg->attachFragmentShader(R"(#version 330 core
            void main(){})");
        instShadowProg->link();
    }
    instShadowProg->use();
    instShadowProg->setUniformMat4("uLS", _lightSpaceMatrix);
    glBindVertexArray(_instancedCubeVAO);
    glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)_cubeMesh->indexCount(),
                            GL_UNSIGNED_INT, 0, (GLsizei)shadowPos.size());
    glBindVertexArray(0);

    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

PhongParams MinecraftGame::makePhongParams(const glm::vec3& camPos) const {
    PhongParams pp;
    pp.sunDir = glm::normalize(_sunDir);
    pp.sunColor = _sunColor;
    pp.ambient = glm::vec3(_ambient);
    pp.specColor = _specColor;
    pp.shininess = _shininess;
    pp.camPos = camPos;
    pp.ptPos = _ptPos;
    pp.ptColor = _ptColor;
    pp.ptIntensity = _ptIntensity;
    pp.ptAttenConst = _ptAttenConst;
    pp.ptAttenLinear = _ptAttenLinear;
    pp.ptAttenQuad = _ptAttenQuad;
    pp.lightSpace = _lightSpaceMatrix;
    pp.shadowMap = _shadowMap;
    pp.shadowBias = _shadowBias;
    pp.shadowsOn = _shadowsOn;
    return pp;
}

// ============================================================
// Texture atlas generation (procedural)
// ============================================================
void MinecraftGame::buildTextureAtlas() {
    // Generate 1024x1024 RGBA atlas, 8x8 tiles of 128x128
    const int SZ = ATLAS_SIZE, TP = TILE_PX, COLS = ATLAS_COLS;
    std::vector<uint8_t> pixels(SZ * SZ * 4, 255);

    auto putTile = [&](int tileIdx, auto gen) {
        int row = tileIdx / COLS, col = tileIdx % COLS;
        for (int py = 0; py < TP; ++py) {
            for (int px = 0; px < TP; ++px) {
                int x = col * TP + px, y = row * TP + py;
                int idx = (y * SZ + x) * 4;
                gen(px, py, pixels[idx], pixels[idx+1], pixels[idx+2], pixels[idx+3]);
            }
        }
    };

    // Hash for variation
    auto h = [](int x, int y) -> uint32_t {
        uint32_t v = (uint32_t)(x * 1271 + y * 91811);
        v = ((v >> 16) ^ v) * 0x45d9f3b;
        return v;
    };

    int ti = 0; // tile index

    // === Grass top (green with dots) ===
    putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r=60+(uint8_t)(h(x,y)%30); g=140+(uint8_t)(h(x,y+1)%40); b=50+(uint8_t)(h(x+2,y)%20); a=255;
    });
    // === Grass bottom (dirt brown) ===
    putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r=120+(uint8_t)(h(x,y)%40); g=80+(uint8_t)(h(x+1,y)%30); b=50+(uint8_t)(h(x,y+2)%25); a=255;
    });
    // === Grass side (green top half, brown bottom) ===
    putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        if(y<TP/2){r=55+(uint8_t)(h(x,y)%25);g=130+(uint8_t)(h(x,y+1)%35);b=45;}
        else{r=115+(uint8_t)(h(x,y)%35);g=75+(uint8_t)(h(x+1,y)%25);b=45;}
        a=255;
    });

    // === Dirt (all same brown) ===
    for(int i=0;i<3;i++) putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r=120+(uint8_t)(h(x,y)%45); g=80+(uint8_t)(h(x+1,y)%35); b=50+(uint8_t)(h(x,y+2)%30); a=255;
    });

    // === Stone (grey with speckles) ===
    for(int i=0;i<3;i++) putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        uint8_t base=120+(uint8_t)(h(x,y)%50);
        r=base; g=base; b=base;
        if(h(x,y+3)%10==0){r=160;g=160;b=160;}
        a=255;
    });

    // === Wood top/bottom (rings) ===
    putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        int cx=TP/2,cy=TP/2; int dx=x-cx,dy=y-cy; float dist=sqrtf((float)(dx*dx+dy*dy));
        int ring=(int)(dist/6)%3;
        r=140-(uint8_t)(ring*20); g=100-(uint8_t)(ring*15); b=60-(uint8_t)(ring*10); a=255;
    });
    putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        int cx=TP/2,cy=TP/2; int dx=x-cx,dy=y-cy; float dist=sqrtf((float)(dx*dx+dy*dy));
        int ring=(int)(dist/6)%3;
        r=140-(uint8_t)(ring*20); g=100-(uint8_t)(ring*15); b=60-(uint8_t)(ring*10); a=255;
    });
    // === Wood side (vertical bark) ===
    putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        int stripe=(x/10)%3;
        r=130+(uint8_t)(stripe==0?20:0); g=90+(uint8_t)(stripe==1?15:0); b=55; a=255;
    });

    // === Leaves (green with gaps) ===
    for(int i=0;i<3;i++) putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        if(h(x,y)%10<2){r=0;g=0;b=0;a=0;} // transparent gaps
        else{r=30+(uint8_t)(h(x,y)%30);g=100+(uint8_t)(h(x+2,y)%50);b=20+(uint8_t)(h(x,y+3)%20);a=255;}
    });

    // === Sand (tan with dots) ===
    for(int i=0;i<3;i++) putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        r=210+(uint8_t)(h(x,y)%25); g=195+(uint8_t)(h(x+1,y)%25); b=130+(uint8_t)(h(x,y+2)%20); a=255;
    });

    // === Water (blue ripple) ===
    for(int i=0;i<3;i++) putTile(ti++, [&](int x, int y, uint8_t& r, uint8_t& g, uint8_t& b, uint8_t& a) {
        float wave=sinf(x*0.08f+y*0.06f)*8.0f;
        r=40+(uint8_t)(wave); g=100+(uint8_t)(wave); b=200+(uint8_t)(wave);
        r=(uint8_t)std::max(0,std::min(255,(int)r));
        g=(uint8_t)std::max(0,std::min(255,(int)g));
        b=(uint8_t)std::max(0,std::min(255,(int)b));
        a=180;
    });

    // Upload to OpenGL
    glGenTextures(1, &_blockAtlas);
    glBindTexture(GL_TEXTURE_2D, _blockAtlas);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SZ, SZ, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
// First-person forward vector
void MinecraftGame::buildEarthTexture() {
    int w, h, c;
    unsigned char* data = stbi_load("media/texture/miscellaneous/earthmap.jpg", &w, &h, &c, 4);
    if (!data) {
        // Fallback: create a blue sphere
        std::vector<uint8_t> fb(64 * 64 * 4, 255);
        for (int i = 0; i < 64 * 64; i++) {
            fb[i*4]=40; fb[i*4+1]=100; fb[i*4+2]=200;
        }
        glGenTextures(1, &_earthTex);
        glBindTexture(GL_TEXTURE_2D, _earthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0, GL_RGBA, GL_UNSIGNED_BYTE, fb.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindTexture(GL_TEXTURE_2D, 0);
        return;
    }
    glGenTextures(1, &_earthTex);
    glBindTexture(GL_TEXTURE_2D, _earthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
}

void MinecraftGame::createShowcaseObjects() {
    auto el = Primitives::CreateSphere(1.5f, 48);
    _earthMesh = std::make_unique<Mesh>(el.vertices, el.indices);

    struct PrimDef { std::function<Element()> fn; };
    PrimDef defs[] = {
        { []{ return Primitives::CreateCube(1.2f); } },
        { []{ return Primitives::CreateSphere(0.8f, 32); } },
        { []{ return Primitives::CreateCylinder(0.7f, 1.5f, 32); } },
        { []{ return Primitives::CreateCone(0.8f, 1.5f, 32); } },
        { []{ return Primitives::CreatePrism(5, 0.8f, 1.2f); } },
        { []{ return Primitives::CreateFrustum(6, 0.9f, 0.4f, 1.2f); } },
    };
    const char* names[] = {"Cube","Sphere","Cylinder","Cone","Prism","Frustum"};
    for (int i = 0; i < 6; i++) {
        auto e = defs[i].fn();
        _primitiveMeshes.push_back(std::make_unique<Mesh>(e.vertices, e.indices));
        _primitiveNames.push_back(names[i]);
    }
}

// ============================================================
glm::vec3 MinecraftGame::fpForward() const {
    float cy = cos(_playerYaw), sy = sin(_playerYaw);
    float cp = cos(_fpPitch), sp = sin(_fpPitch);
    return glm::vec3(-cp * sy, sp, -cp * cy);
}

// ============================================================
// Input — dispatch
// ============================================================
void MinecraftGame::handleInput() {
    _gameTime += _deltaTime;
    if (_screenshotFlash > 0) _screenshotFlash -= _deltaTime;

    // Smooth FPS: exponential moving average
    if (_deltaTime > 0) {
        float instantFPS = 1.0f / _deltaTime;
        _smoothFPS = _smoothFPS * 0.9f + instantFPS * 0.1f;
    }
    updateDroppedItems();

    // ---- F2: screenshot (PNG) ----
    static bool f2WasDown = false;
    bool f2Now = (_input.keyboard.keyStates[GLFW_KEY_F2] != GLFW_RELEASE);
    if (f2Now && !f2WasDown) {
        _mkdir("screenshots");
        std::vector<uint8_t> pixels(_windowWidth * _windowHeight * 3);
        glReadPixels(0, 0, _windowWidth, _windowHeight, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
        // Flip vertically (OpenGL bottom-left vs PNG top-left)
        std::vector<uint8_t> flipped(_windowWidth * _windowHeight * 3);
        for (int y = 0; y < _windowHeight; ++y)
            memcpy(&flipped[y * _windowWidth * 3],
                   &pixels[(_windowHeight - 1 - y) * _windowWidth * 3],
                   _windowWidth * 3);
        char fname[128];
        time_t now = time(nullptr);
        snprintf(fname, sizeof(fname), "screenshots/screenshot_%lld.png", (long long)now);
        stbi_write_png(fname, _windowWidth, _windowHeight, 3, flipped.data(), _windowWidth * 3);
        _screenshotFlash = 2.0f;
    }
    f2WasDown = f2Now;

    // ---- L: toggle settings ----
    static bool lWasDown = false;
    bool lNow = (_input.keyboard.keyStates[GLFW_KEY_L] != GLFW_RELEASE);
    if (lNow && !lWasDown) _settingsOpen = !_settingsOpen;
    lWasDown = lNow;

    if (_inventoryOpen) {
        _input.mouse.scroll.xOffset = 0;
        _input.mouse.scroll.yOffset = 0;
        return;
    }

    _ctrl.begin(_input);
    if (_firstPerson)
        handleInputFirstPerson();
    else
        handleInputThirdPerson();
    _input.mouse.scroll.xOffset = 0;
    _input.mouse.scroll.yOffset = 0;
}
// ============================================================
// First-person input
// ============================================================
void MinecraftGame::handleInputFirstPerson() {
    // ---- V: toggle back to 3rd ----
    static bool vWasDown = false;
    bool vNow = _input.keyboard.keyStates[GLFW_KEY_V] != GLFW_RELEASE;
    if (vNow && !vWasDown) {
        _firstPerson = false;
        _fc.yaw = glm::degrees(_playerYaw);
        _fc.pitch = glm::degrees(_fpPitch);
    }
    vWasDown = vNow;

    // ---- FPS mouse look ----
    float mdx = (float)(_input.mouse.move.xNow - _prevMX);
    float mdy = (float)(_input.mouse.move.yNow - _prevMY);
    _playerYaw -= mdx * 0.008f;
    _fpPitch   -= mdy * 0.008f;
    if (_fpPitch >  1.5f) _fpPitch =  1.5f;
    if (_fpPitch < -1.5f) _fpPitch = -1.5f;
    _prevMX = _input.mouse.move.xNow;
    _prevMY = _input.mouse.move.yNow;

    // ---- Movement ----
    float speed = 5.0f * _deltaTime;
    bool moving = false;
    glm::vec3 fwd = fpForward();
    glm::vec3 right = glm::normalize(glm::cross(fwd, glm::vec3(0, 1, 0)));
    glm::vec3 moveDir(0);
    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) { moveDir += glm::vec3(fwd.x, 0, fwd.z); moving = true; }
    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) { moveDir -= glm::vec3(fwd.x, 0, fwd.z); moving = true; }
    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) { moveDir -= right; moving = true; }
    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) { moveDir += right; moving = true; }
    if (moving) {
        float len = glm::length(moveDir);
        if (len > 0.001f) { moveDir /= len; }
        _onGround = resolvePlayerMovement(_playerPos, moveDir * speed);
    }

    // ---- Jump / Gravity ----
    if (_input.keyboard.keyStates[GLFW_KEY_SPACE] != GLFW_RELEASE && _onGround && !_jumping) {
        _playerYVel = 8.0f; _onGround = false; _jumping = true; _jumpTimer = 0;
    }
    if (_jumping) {
        _playerYVel -= 12.0f * _deltaTime;
        _onGround = resolvePlayerMovement(_playerPos, glm::vec3(0, _playerYVel * _deltaTime, 0));
        if (_onGround) _playerYVel = 0;
        _jumpTimer += _deltaTime;
        if (_jumpTimer > 0.6f) { _jumping = false; _jumpTimer = 0; }
    }
    if (!_onGround && !_jumping) {
        _playerYVel -= 12.0f * _deltaTime;
        _onGround = resolvePlayerMovement(_playerPos, glm::vec3(0, _playerYVel * _deltaTime, 0));
        if (_onGround) _playerYVel = 0;
    }

    // ---- Animation ----
    if (_jumping) {
        if (_animState != JUMP) { _animator.crossFade(&_builder.jumpClip, 0.15f); _animState = JUMP; }
    } else if (moving) {
        if (_animState != WALK) { _animator.crossFade(&_builder.walkClip, 0.25f); _animState = WALK; }
    } else {
        if (_animState != IDLE) { _animator.crossFade(&_builder.idleClip, 0.30f); _animState = IDLE; }
    }

    // ---- Raycast ----
    computeGhost();

    // ---- Left click = place block ----
    static bool fpLeftWas = false;
    bool fpLeft = _input.mouse.press.left;
    if (!fpLeft && fpLeftWas && !ImGui::GetIO().WantCaptureMouse) {
        if (_ghostPlaceValid) {
            // Don't place if it would trap the player
            AABB blockBox(glm::vec3(_ghostPlace), glm::vec3(0.5f));
            AABB playerBox = AABB::fromMinMax(
                _playerPos - glm::vec3(PLAYER_HALF_W, 0, PLAYER_HALF_W),
                _playerPos + glm::vec3(PLAYER_HALF_W, PLAYER_HALF_H * 2.0f, PLAYER_HALF_W));
            _blocks[_ghostPlace] = _selectedBlock;
            if (blockBox.overlaps(playerBox))
                _playerPos.y = (float)(_ghostPlace.y + 1);
        }
    }
    fpLeftWas = fpLeft;
    // ---- Right click = remove block ----
    static bool fpRightWas = false;
    bool fpRight = _input.mouse.press.right;
    if (!fpRight && fpRightWas && !ImGui::GetIO().WantCaptureMouse) {
        if (_ghostHitValid) {
            int bt = _blocks[_ghostHit];
            spawnDroppedItem(_ghostHit, bt);
            _blocks.erase(_ghostHit);
        }
    }
    fpRightWas = fpRight;
}

// ============================================================
// Third-person input
// ============================================================
void MinecraftGame::handleInputThirdPerson() {
    // ---- V: toggle to 1st ----
    static bool vWasDown = false;
    bool vNow = _input.keyboard.keyStates[GLFW_KEY_V] != GLFW_RELEASE;
    if (vNow && !vWasDown) {
        _firstPerson = true;
        _fpPitch = glm::radians(_fc.pitch);
        _playerYaw = glm::radians(_fc.yaw);
        _prevMX = _input.mouse.move.xNow;
        _prevMY = _input.mouse.move.yNow;
    }
    vWasDown = vNow;

    // ---- Movement ----
    float speed = 4.0f * _deltaTime;
    bool moving = false;
    float ry = glm::radians(_fc.yaw);
    glm::vec3 camFwd = glm::vec3(-cos(ry), 0, -sin(ry));
    glm::vec3 camRight = glm::normalize(glm::cross(camFwd, glm::vec3(0, 1, 0)));
    glm::vec3 moveDir(0);
    if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) { moveDir += camFwd;  moving = true; }
    if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) { moveDir -= camFwd;  moving = true; }
    if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) { moveDir -= camRight; moving = true; }
    if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) { moveDir += camRight; moving = true; }
    if (moving) {
        float len = glm::length(moveDir);
        if (len > 0.001f) {
            moveDir /= len;
            _playerYaw = atan2(moveDir.x, moveDir.z);
        }
        _onGround = resolvePlayerMovement(_playerPos, moveDir * speed);
    }

    // ---- Jump / Gravity ----
    if (_input.keyboard.keyStates[GLFW_KEY_SPACE] != GLFW_RELEASE && _onGround && !_jumping) {
        _playerYVel = 8.0f; _onGround = false; _jumping = true; _jumpTimer = 0;
    }
    if (_jumping) {
        _playerYVel -= 12.0f * _deltaTime;
        _onGround = resolvePlayerMovement(_playerPos, glm::vec3(0, _playerYVel * _deltaTime, 0));
        if (_onGround) _playerYVel = 0;
        _jumpTimer += _deltaTime;
        if (_jumpTimer > 0.6f) { _jumping = false; _jumpTimer = 0; }
    }
    if (!_onGround && !_jumping) {
        _playerYVel -= 12.0f * _deltaTime;
        _onGround = resolvePlayerMovement(_playerPos, glm::vec3(0, _playerYVel * _deltaTime, 0));
        if (_onGround) _playerYVel = 0;
    }

    // ---- Animation ----
    if (_jumping) {
        if (_animState != JUMP) { _animator.crossFade(&_builder.jumpClip, 0.15f); _animState = JUMP; }
    } else if (moving) {
        if (_animState != WALK) { _animator.crossFade(&_builder.walkClip, 0.25f); _animState = WALK; }
    } else {
        if (_animState != IDLE) { _animator.crossFade(&_builder.idleClip, 0.30f); _animState = IDLE; }
    }

    // ---- Camera follow ----
    _fc.target = _playerPos + glm::vec3(0, 1.0f, 0);

    // Right-drag orbit: track start, only orbit after 4px movement
    if (_input.mouse.press.right) {
        if (_rightStartMX == 0 && _rightStartMY == 0) {
            _rightStartMX = _input.mouse.move.xNow;
            _rightStartMY = _input.mouse.move.yNow;
        }
        float drag = fabsf(_input.mouse.move.xNow - _rightStartMX)
                   + fabsf(_input.mouse.move.yNow - _rightStartMY);
        if (drag > 4.0f) _ctrl.orbit();
    } else if (_rightStartMX != 0 || _rightStartMY != 0) {
        // Right released: check for click (not drag) → remove block
        float drag = fabsf(_input.mouse.move.xNow - _rightStartMX)
                   + fabsf(_input.mouse.move.yNow - _rightStartMY);
        if (drag < 4.0f && _ghostHitValid && !ImGui::GetIO().WantCaptureMouse) {
            int bt = _blocks[_ghostHit];
            spawnDroppedItem(_ghostHit, bt);
            _blocks.erase(_ghostHit);
        }
        _rightStartMX = 0; _rightStartMY = 0;
    }
    _ctrl.zoom();
    _ctrl.pan();

    // ---- F: toggle zoom to fit / restore ----
    static bool fWasDown = false;
    bool fNow = (_input.keyboard.keyStates[GLFW_KEY_F] != GLFW_RELEASE);
    if (fNow && !fWasDown) {
        if (_zoomedOut) {
            _fc.dist  = _preZoomDist;
            _fc.yaw   = _preZoomYaw;
            _fc.pitch = _preZoomPitch;
            _fc.target = _preZoomTarget;
            _zoomedOut = false;
        } else {
            _preZoomDist   = _fc.dist;
            _preZoomYaw    = _fc.yaw;
            _preZoomPitch  = _fc.pitch;
            _preZoomTarget = _fc.target;
            _fc.target = _playerPos + glm::vec3(0, 1.0f, 0);
            _fc.zoomToFit((float)WORLD_SIZE);
            _zoomedOut = true;
        }
    }
    fWasDown = fNow;

    // ---- Raycast ----
    computeGhost();

    // ---- Left click = place block ----
    static bool leftWasDown = false;
    bool leftNow = _input.mouse.press.left;
    if (!leftNow && leftWasDown && !ImGui::GetIO().WantCaptureMouse) {
        if (_ghostPlaceValid) {
            AABB blockBox(glm::vec3(_ghostPlace), glm::vec3(0.5f));
            AABB playerBox = AABB::fromMinMax(
                _playerPos - glm::vec3(PLAYER_HALF_W, 0, PLAYER_HALF_W),
                _playerPos + glm::vec3(PLAYER_HALF_W, PLAYER_HALF_H * 2.0f, PLAYER_HALF_W));
            _blocks[_ghostPlace] = _selectedBlock;
            if (blockBox.overlaps(playerBox))
                _playerPos.y = (float)(_ghostPlace.y + 1);
        }
    }
    leftWasDown = leftNow;
}

// ============================================================
// Ghost block preview
// ============================================================
void MinecraftGame::computeGhost() {
    _ghostHitValid = false;
    _ghostPlaceValid = false;
    glm::vec3 origin, dir;
    if (_firstPerson) {
        origin = _playerPos + glm::vec3(0, 1.7f, 0);
        dir = fpForward();
    } else {
        // Third-person: unproject mouse cursor to world ray
        float mx = glm::clamp(_input.mouse.move.xNow, 0.0f, (float)_windowWidth - 1.0f);
        float my = (float)_windowHeight - glm::clamp(_input.mouse.move.yNow, 0.0f, (float)_windowHeight - 1.0f);
        auto proj = glm::perspective(glm::radians(60.f), (float)_windowWidth / _windowHeight, 0.1f, 200.f);
        auto view = _fc.viewMatrix();
        glm::vec4 vp(0, 0, (float)_windowWidth, (float)_windowHeight);
        glm::vec3 nearW = glm::unProject(glm::vec3(mx, my, 0.0f), view, proj, vp);
        glm::vec3 farW  = glm::unProject(glm::vec3(mx, my, 1.0f), view, proj, vp);
        origin = nearW;
        dir = glm::normalize(farW - nearW);
    }
    for (float t = 0.5f; t < 40.0f; t += 0.5f) {
        glm::ivec3 gp = glm::ivec3(glm::round(origin + dir * t));
        auto it = _blocks.find(gp);
        if (it != _blocks.end() && it->second != BT_WATER) {
            _ghostHit = gp;
            _ghostHitValid = true;
            // Compute adjacent empty position (for placement preview)
            glm::vec3 hp = origin + dir * t;
            glm::vec3 hn = glm::vec3(gp) - hp;
            float ax = fabsf(hn.x), ay = fabsf(hn.y), az = fabsf(hn.z);
            glm::ivec3 adj = gp;
            if (ax >= ay && ax >= az)      adj.x += (hn.x > 0 ? -1 : 1);
            else if (ay >= ax && ay >= az) adj.y += (hn.y > 0 ? -1 : 1);
            else                           adj.z += (hn.z > 0 ? -1 : 1);
            if (_blocks.find(adj) == _blocks.end()) {
                _ghostPlace = adj;
                _ghostPlaceValid = true;
            }
            return;
        }
    }
}

void MinecraftGame::drawGhost(const glm::mat4& proj, const glm::mat4& view) {
    if (!_ghostPlaceValid) return;
    auto model = glm::translate(glm::mat4(1), glm::vec3(_ghostPlace));

    glm::vec3 camPos;
    if (_firstPerson) camPos = _playerPos + glm::vec3(0, 1.7f, 0);
    else camPos = _fc.position();

    PhongParams pp = makePhongParams(camPos);
    pp.shadowsOn = false; // No shadows on ghost preview

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.0f);
    drawLit(_litShader, _cubeMesh.get(), model, blockTypeColor(_selectedBlock), proj, view, pp);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
    glDisable(GL_BLEND);
}

// ============================================================
// Dropped items
// ============================================================
void MinecraftGame::spawnDroppedItem(const glm::ivec3& blockPos, int bt) {
    DroppedItem di;
    di.pos = glm::vec3(blockPos) + glm::vec3(0.5f, 0.25f, 0.5f);
    di.blockType = bt;
    di.bobPhase = (float)(hash((uint32_t)(blockPos.x * 131 + blockPos.y * 271 + blockPos.z * 397)) % 628) / 100.0f;
    _droppedItems.push_back(di);
}

void MinecraftGame::updateDroppedItems() {
    glm::vec3 playerCenter = _playerPos + glm::vec3(0, 1.0f, 0);

    for (auto it = _droppedItems.begin(); it != _droppedItems.end(); ) {
        it->lifeTime += _deltaTime;

        float dx = it->pos.x - playerCenter.x;
        float dy = it->pos.y - playerCenter.y;
        float dz = it->pos.z - playerCenter.z;
        float distSq = dx * dx + dy * dy + dz * dz;

        // Start pickup animation
        if (!it->beingPickedUp && distSq < PICKUP_RANGE * PICKUP_RANGE) {
            it->beingPickedUp = true;
            it->pickupTimer = 0;
            it->pickupStartPos = it->pos;
        }

        // Animate pickup: fly toward player over 0.35s
        if (it->beingPickedUp) {
            it->pickupTimer += _deltaTime / 0.35f;
            if (it->pickupTimer >= 1.0f) {
                it = _droppedItems.erase(it);
                continue;
            }
            // Smoothstep pickup animation
            float t = it->pickupTimer;
            t = t * t * (3.0f - 2.0f * t);
            it->pos = glm::mix(it->pickupStartPos, playerCenter, t);
        }
        ++it;
    }
}
void MinecraftGame::drawDroppedItems(const glm::mat4& proj, const glm::mat4& view) {
    if (_droppedItems.empty()) return;
    float itemScale = 0.2f;
    auto baseScale = glm::scale(glm::mat4(1), glm::vec3(itemScale));

    PhongParams pp = makePhongParams(
        _firstPerson ? (_playerPos + glm::vec3(0, 1.7f, 0)) : _fc.position());

    for (auto& di : _droppedItems) {
        // Shrink during pickup
        float scale = 1.0f;
        if (di.beingPickedUp)
            scale = 1.0f - di.pickupTimer * 0.7f;

        // Bobbing (only when not being picked up)
        float bob = di.beingPickedUp ? 0.0f : sinf(di.lifeTime * 4.0f + di.bobPhase) * 0.08f;
        float spin = di.beingPickedUp ? di.lifeTime * 6.0f : di.lifeTime * 1.5f;

        auto model = glm::translate(glm::mat4(1), di.pos + glm::vec3(0, bob, 0))
                   * glm::rotate(glm::mat4(1), spin, glm::vec3(0, 1, 0))
                   * glm::scale(baseScale, glm::vec3(scale));

        int baseTile = (di.blockType - 1) * 3;
        drawTexturedLit(_texLitShader, _cubeMesh.get(), model, baseTile,
            (float)ATLAS_COLS, _blockAtlas, proj, view, pp);
    }
}

// ============================================================
// Lighting helper
// ============================================================
void MinecraftGame::setLight() {
    _shader.setLight(_sunDir, _sunColor, _ptPos, _ptColor, _ptIntensity);
    _shader.setAmbient(_ambient);
}

// ============================================================
// Collision resolution
// ============================================================
bool MinecraftGame::resolvePlayerMovement(glm::vec3& pos, const glm::vec3& desired) {
    AABB box = AABB::fromMinMax(
        pos - glm::vec3(PLAYER_HALF_W, 0, PLAYER_HALF_W),
        pos + glm::vec3(PLAYER_HALF_W, PLAYER_HALF_H * 2.0f, PLAYER_HALF_W)
    );
    bool onGround;
    glm::vec3 resolved = collideAndSlide(box, desired, _blocks, onGround,
        [](int bt) { return bt != BT_WATER; });
    pos += resolved;
    return onGround;
}
// ============================================================
// Block queries
// ============================================================
int MinecraftGame::getBlock(int x, int y, int z) const {
    auto it = _blocks.find(glm::ivec3(x, y, z));
    return (it != _blocks.end()) ? it->second : BT_AIR;
}
bool MinecraftGame::isSolidBlock(int x, int y, int z) const {
    int bt = getBlock(x, y, z);
    return bt != BT_AIR && bt != BT_WATER && bt != BT_LEAVES;
}
// ============================================================
// Render — instanced per block type
// ============================================================
void MinecraftGame::drawBlocks(const glm::mat4& proj, const glm::mat4& view) {
    glm::vec3 camPos;
    if (_firstPerson) camPos = _playerPos + glm::vec3(0, 1.7f, 0);
    else              camPos = _fc.position();

    PhongParams pp = makePhongParams(camPos);

    // Collect instance positions per block type
    static std::vector<glm::vec3> posBuf[BT_COUNT];
    for (int bt = 1; bt < BT_COUNT; ++bt) posBuf[bt].clear();

    for (auto& kv : _blocks) {
        if (kv.second == BT_AIR) continue;
        posBuf[kv.second].push_back(glm::vec3(kv.first));
    }

    int indexCount = (int)_cubeMesh->indexCount();
    // Opaque pass
    for (int bt = 1; bt < BT_COUNT; ++bt) {
        if (bt == BT_WATER || bt == BT_LEAVES) continue;
        if (posBuf[bt].empty()) continue;

        glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, posBuf[bt].size() * sizeof(glm::vec3),
                     posBuf[bt].data(), GL_DYNAMIC_DRAW);

        int baseTile = (bt - 1) * 3;
        drawTexturedLitInstanced(_instTexLitShader, _instancedCubeVAO,
            indexCount, (int)posBuf[bt].size(), baseTile,
            (float)ATLAS_COLS, _blockAtlas, proj, view, pp);
    }

    // Transparent pass
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    int transTypes[] = {BT_WATER, BT_LEAVES};
    for (int ti = 0; ti < 2; ++ti) {
        int bt = transTypes[ti];
        if (posBuf[bt].empty()) continue;

        glBindBuffer(GL_ARRAY_BUFFER, _instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, posBuf[bt].size() * sizeof(glm::vec3),
                     posBuf[bt].data(), GL_DYNAMIC_DRAW);

        int baseTile = (bt - 1) * 3;
        drawTexturedLitInstanced(_instTexLitShader, _instancedCubeVAO,
            indexCount, (int)posBuf[bt].size(), baseTile,
            (float)ATLAS_COLS, _blockAtlas, proj, view, pp);
    }
    glDisable(GL_BLEND);
}

void MinecraftGame::drawCharacter(const glm::mat4& proj, const glm::mat4& view) {
    auto model = glm::translate(glm::mat4(1), _playerPos + glm::vec3(0, 0.78f, 0))
               * glm::rotate(glm::mat4(1), _playerYaw, glm::vec3(0, 1, 0));
    _shader.use();
    _shader.setMatrices(proj, view, model);
    _shader.setBoneMatrices(_builder.skeleton().finalMatrices());
    _shader.setColor(glm::vec3(0.5f, 0.7f, 0.5f));
    setLight();
    _mesh->draw();
}

// ============================================================
// UI
// ============================================================
void MinecraftGame::drawUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (_inventoryOpen) {
        drawInventory();
    } else {
        // Hotbar — bottom center
        int placableCount = BT_COUNT - 1;
        float btnW = 80.0f;
        float gap = 6.0f;
        float barW = placableCount * btnW + (placableCount - 1) * gap + 20;
        ImGui::SetNextWindowPos(ImVec2(_windowWidth * 0.5f - barW * 0.5f, (float)_windowHeight - 55), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(barW, 45));
        ImGui::Begin("Hotbar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

        int bt = BT_GRASS;
        for (int i = 0; i < placableCount; i++) {
            bt = BT_GRASS + i;
            if (i > 0) ImGui::SameLine();
            ImGui::PushID(i);
            glm::vec3 bc = blockTypeColor(bt);
            ImVec4 c(bc.x, bc.y, bc.z, 1);
            ImGui::PushStyleColor(ImGuiCol_Button, c);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.x * 1.15f, c.y * 1.15f, c.z * 1.15f, 1));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(c.x * 0.8f, c.y * 0.8f, c.z * 0.8f, 1));
            char label[32];
            snprintf(label, sizeof(label), "%s%s", _selectedBlock == bt ? "[*]" : "", blockTypeName(bt));
            if (ImGui::Button(label, ImVec2(btnW, 30)))
                _selectedBlock = bt;
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
        ImGui::End();
        ImGui::Text("WASD=Move Space=Jump | V=1st/3rd | E=Bag | F2=Screen");
        ImGui::Text("FPS: %.0f", _smoothFPS);
        if (_screenshotFlash > 0) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));
            ImGui::Text("Screenshot saved!");
            ImGui::PopStyleColor();
        }
        ImGui::Text("LClick=Place (%s) | RClick=Destroy", blockTypeName(_selectedBlock));
        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::Begin("Info", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar);
        ImGui::Text("WASD=Move Space=Jump | V=1st/3rd | E=Bag | F2=Screen");
        ImGui::Text("LClick=Place (%s) | RClick=Destroy", blockTypeName(_selectedBlock));
        if (!_firstPerson)
            ImGui::Text("[3rd] RightDrag=Orbit Scroll=Zoom | MButton=Pan | F=ZoomFit");
        else
            ImGui::Text("[1st] Mouse=Look");
        ImGui::End();
    }

    // Bag button — bottom-right, outside hotbar area
    {
        ImGui::SetNextWindowPos(ImVec2((float)_windowWidth - 85, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(75, 30));
        ImGui::Begin("InvBtn", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        if (ImGui::Button(_inventoryOpen ? "Close" : "Bag[E]", ImVec2(65, 20)))
            _inventoryOpen = !_inventoryOpen;
        ImGui::End();
    }

    // Settings button
    {
        ImGui::SetNextWindowPos(ImVec2((float)_windowWidth - 85, 45), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(75, 30));
        ImGui::Begin("SetBtn", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        if (ImGui::Button(_settingsOpen ? "Hide[L]" : "Set[L]", ImVec2(65, 20)))
            _settingsOpen = !_settingsOpen;
        ImGui::End();
    }

    // Lighting panel — only when settings open
    if (_settingsOpen) {
        ImGui::SetNextWindowPos(ImVec2((float)_windowWidth - 285, 85), ImGuiCond_FirstUseEver);
        ImGui::Begin("Settings (Phong Lighting)", &_settingsOpen, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Directional Sun");
        ImGui::SliderFloat("Sun X", &_sunDir.x, -1.0f, 1.0f);
        ImGui::SliderFloat("Sun Y", &_sunDir.y, 0.1f, 1.0f);
        ImGui::SliderFloat("Sun Z", &_sunDir.z, -1.0f, 1.0f);
        ImGui::ColorEdit3("Sun Color", &_sunColor[0]);

        ImGui::Separator();
        ImGui::Text("Phong Parameters");
        ImGui::SliderFloat("Ambient", &_ambient, 0.0f, 0.8f);
        ImGui::SliderFloat("Shininess", &_shininess, 1.0f, 256.0f, "%.0f");
        ImGui::ColorEdit3("Spec Color", &_specColor[0]);

        ImGui::Separator();
        ImGui::Text("Point Light");
        ImGui::SliderFloat("Pt Pos X", &_ptPos.x, -20.0f, 20.0f);
        ImGui::SliderFloat("Pt Pos Y", &_ptPos.y, 0.5f, 20.0f);
        ImGui::SliderFloat("Pt Pos Z", &_ptPos.z, -20.0f, 20.0f);
        ImGui::SliderFloat("Pt Intensity", &_ptIntensity, 0.0f, 20.0f);
        ImGui::ColorEdit3("Pt Color", &_ptColor[0]);
        ImGui::Text("Attenuation (C+L*d+Q*d^2)");
        ImGui::SliderFloat("Atten Const", &_ptAttenConst, 0.1f, 3.0f);
        ImGui::SliderFloat("Atten Linear", &_ptAttenLinear, 0.0f, 0.5f, "%.3f");
        ImGui::SliderFloat("Atten Quad", &_ptAttenQuad, 0.0f, 0.2f, "%.3f");

        ImGui::Separator();
        ImGui::Text("Shadow Mapping");
        ImGui::Checkbox("Shadows", &_shadowsOn);
        ImGui::SliderFloat("Bias", &_shadowBias, 0.0001f, 0.01f, "%.4f");

        if (ImGui::Button("Reset Light")) {
            _sunDir = {0.5f, 1.0f, 0.3f};
            _sunColor = {1.0f, 0.95f, 0.85f};
            _ambient = 0.3f;
            _shininess = 32.0f;
            _specColor = {1.0f, 1.0f, 1.0f};
            _ptPos = {3, 4, -2};
            _ptColor = {0.3f, 0.5f, 1.0f};
            _ptIntensity = 5.0f;
            _ptAttenConst = 1.0f;
            _ptAttenLinear = 0.09f;
            _ptAttenQuad = 0.032f;
            _shadowBias = 0.0005f;
            _shadowsOn = true;
        }
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void MinecraftGame::drawInventory() {
    float invW = 400, invH = 300;
    ImGui::SetNextWindowPos(ImVec2(_windowWidth * 0.5f - invW * 0.5f, _windowHeight * 0.5f - invH * 0.5f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(invW, invH));
    ImGui::Begin("Inventory (press E to close)", &_inventoryOpen,
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

    int cols = 4;
    int placableCount = BT_COUNT - 1; // exclude AIR

    for (int i = 0; i < placableCount; i++) {
        int bt = BT_GRASS + i;
        if (i % cols != 0) ImGui::SameLine();

        ImGui::PushID(i);
        glm::vec3 bc = blockTypeColor(bt);
        ImVec4 c(bc.x, bc.y, bc.z, 1);
        ImGui::PushStyleColor(ImGuiCol_Button, c);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(c.x * 1.2f, c.y * 1.2f, c.z * 1.2f, 1));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(c.x * 0.7f, c.y * 0.7f, c.z * 0.7f, 1));

        char label[48];
        snprintf(label, sizeof(label), "%s%s\n%s",
            _selectedBlock == bt ? "> " : "  ",
            blockTypeName(bt),
            _selectedBlock == bt ? "[selected]" : "");
        if (ImGui::Button(label, ImVec2(85, 65)))
            _selectedBlock = bt;

        ImGui::PopStyleColor(3);
        ImGui::PopID();
    }

    ImGui::End();
}

// ============================================================
void MinecraftGame::renderFrame() {
    _animator.update(_deltaTime);

    // === Shadow pass ===
    renderShadowPass();

    // Restore viewport
    glViewport(0, 0, _windowWidth, _windowHeight);
    glClearColor(0.45f, 0.65f, 0.85f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    auto proj = glm::perspective(glm::radians(60.f),
        (float)_windowWidth / _windowHeight, 0.1f, 200.f);

    glm::mat4 view;
    if (_firstPerson) {
        glm::vec3 eye = _playerPos + glm::vec3(0, 1.7f, 0)
                      + glm::vec3(fpForward().x, 0, fpForward().z) * 0.15f;
        view = glm::lookAt(eye, eye + fpForward(), glm::vec3(0, 1, 0));
    } else {
        view = _fc.viewMatrix();
    }

    // Skybox
    glDepthFunc(GL_LEQUAL);
    _skybox->draw(proj, glm::mat4(glm::mat3(view)));
    glDepthFunc(GL_LESS);

    // World blocks
    drawBlocks(proj, view);

    // Ghost preview
    drawGhost(proj, view);

    // Dropped items (small floating blocks)
    drawDroppedItems(proj, view);

    // Character
    drawCharacter(proj, view);

    // === Earth globe ===
    {
        glm::vec3 camPos = _firstPerson ? (_playerPos + glm::vec3(0, 1.7f, 0)) : _fc.position();
        glm::vec3 earthPos(0, 10.0f, 0);
        auto m = glm::translate(glm::mat4(1), earthPos) * glm::rotate(glm::mat4(1), _gameTime*0.3f, glm::vec3(0,1,0)) * glm::rotate(glm::mat4(1), -0.4f, glm::vec3(1,0,0));
        PhongParams ep = makePhongParams(camPos); ep.shadowsOn = _shadowsOn;
        drawTexturedLit(_texLitShader, _earthMesh.get(), m, 0, 1.0f, _earthTex, proj, view, ep);
    }

    // === Primitives orbiting Earth ===
    {
        glm::vec3 camPos = _firstPerson ? (_playerPos + glm::vec3(0, 1.7f, 0)) : _fc.position();
        glm::vec3 earthPos(0, 10.0f, 0);
        int n = (int)_primitiveMeshes.size();
        float orbitR = 3.0f;
        const float TPI = 6.283185307f;
        glm::vec4 cols[] = {{0.9f,0.3f,0.3f,1},{0.3f,0.5f,0.9f,1},{0.3f,0.9f,0.4f,1},{0.9f,0.7f,0.2f,1},{0.7f,0.3f,0.9f,1},{0.9f,0.5f,0.5f,1}};
        for(int i=0;i<n;i++){
            float a = _gameTime*0.6f + (float)i/n*TPI;
            float tilt = (float)i * 0.5f;
            float rx = cosf(a) * orbitR;
            float rz = sinf(a) * orbitR * cosf(tilt);
            float ry = sinf(a) * orbitR * sinf(tilt);
            glm::vec3 p = earthPos + glm::vec3(rx, ry, rz);
            auto pm = glm::translate(glm::mat4(1), p)
                     * glm::rotate(glm::mat4(1), _gameTime*0.8f+(float)i, glm::vec3(0.3f,1,0.2f));
            PhongParams pp = makePhongParams(camPos); pp.shadowsOn = _shadowsOn;
            drawLit(_litShader, _primitiveMeshes[i].get(), pm, cols[i], proj, view, pp);
        }
    }

    // Point light indicator — small bright cube at light position
    {
        auto ptModel = glm::translate(glm::mat4(1), _ptPos)
                     * glm::scale(glm::mat4(1), glm::vec3(0.2f));
        PhongParams ptPP;
        ptPP.sunColor = glm::vec3(0);  // no sun
        ptPP.ambient = glm::vec3(0.8f);
        ptPP.camPos = _firstPerson ? (_playerPos + glm::vec3(0, 1.7f, 0)) : _fc.position();
        ptPP.shadowsOn = false;
        drawLit(_litShader, _cubeMesh.get(), ptModel, _ptColor * 2.0f, proj, view, ptPP);
    }

    // UI
    drawUI();
}
