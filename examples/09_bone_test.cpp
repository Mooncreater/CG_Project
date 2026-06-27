// 09_bone_test: absolute minimal skinned mesh test
#include "../src/base/application.h"
#include "../src/animation/skinned_mesh.h"
#include "../src/animation/skinned_shader.h"
#include "../src/orbit_control/orbit_control.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

struct Demo : Application {
    std::unique_ptr<SkinnedMesh> _mesh;
    SkinnedShader _shader;
    FreeCamera _fc; OrbitControl _ctrl{_fc};

    Demo(const Options& o) : Application(o) {
        std::cout << "Creating skinned cube..." << std::endl;

        // Create a simple colored cube with all verts weighted to bone 0
        std::vector<SkinnedVertex> verts;
        float s = 0.5f;
        auto add = [&](glm::vec3 p, glm::vec3 n) {
            SkinnedVertex v; v.position = p; v.normal = n;
            v.boneIds[0]=0; v.boneIds[1]=v.boneIds[2]=v.boneIds[3]=0;
            v.weights[0]=1; v.weights[1]=v.weights[2]=v.weights[3]=0;
            verts.push_back(v);
        };
        // 6 faces, 4 vertices each = 24 verts
        add({-s, s,-s},{0,1,0});add({ s, s,-s},{0,1,0});add({ s, s, s},{0,1,0});add({-s, s, s},{0,1,0});
        add({-s,-s, s},{0,-1,0});add({ s,-s, s},{0,-1,0});add({ s,-s,-s},{0,-1,0});add({-s,-s,-s},{0,-1,0});
        add({ s, s, s},{1,0,0});add({ s, s,-s},{1,0,0});add({ s,-s,-s},{1,0,0});add({ s,-s, s},{1,0,0});
        add({-s, s,-s},{-1,0,0});add({-s, s, s},{-1,0,0});add({-s,-s, s},{-1,0,0});add({-s,-s,-s},{-1,0,0});
        add({-s, s, s},{0,0,1});add({ s, s, s},{0,0,1});add({ s,-s, s},{0,0,1});add({-s,-s, s},{0,0,1});
        add({ s, s,-s},{0,0,-1});add({-s, s,-s},{0,0,-1});add({-s,-s,-s},{0,0,-1});add({ s,-s,-s},{0,0,-1});

        std::vector<uint32_t> idx;
        for(int f=0;f<6;f++){uint32_t b=(uint32_t)(f*4);idx.insert(idx.end(),{b,b+1,b+2,b,b+2,b+3});}

        _mesh = std::make_unique<SkinnedMesh>(verts, idx);
        _fc.target = glm::vec3(0,0,0); _fc.dist = 3;
    }

    void handleInput() override { _ctrl.begin(_input); _ctrl.update(); _input.mouse.scroll.xOffset=0; _input.mouse.scroll.yOffset=0; }

    void renderFrame() override {
        glClearColor(0.1f,0.12f,0.16f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);

        auto proj=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.1f,50.f);
        auto view=_fc.viewMatrix();
        auto model=glm::mat4(1);

        // Bone 0 = identity
        glm::mat4 bones[20] = {};
        bones[0] = glm::mat4(1);

        _shader.use();
        _shader.setMatrices(proj, view, model);
        _shader.setBoneMatrices(bones, 20);
        _shader.setColor(glm::vec3(0.8f, 0.4f, 0.2f));
        _shader.setLight(glm::vec3(0.5f,1,0.3f), glm::vec3(1,0.95f,0.85f), glm::vec3(3,4,-2), glm::vec3(0.3f,0.5f,1), 5);
        _shader.setAmbient(0.3f);
        _mesh->draw();
    }
};

int main(){Options o;o.windowTitle="09 Bone Test";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
