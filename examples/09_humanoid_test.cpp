// 09_humanoid_test: render humanoid mesh with all bones=identity (no skeleton hierarchy)
#include "../src/base/application.h"
#include "../src/animation/skinned_mesh.h"
#include "../src/animation/skinned_shader.h"
#include "../src/animation/humanoid.h"
#include "../src/orbit_control/orbit_control.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

struct Demo : Application {
    HumanoidBuilder _builder;
    std::unique_ptr<SkinnedMesh> _mesh;
    SkinnedShader _shader;
    FreeCamera _fc; OrbitControl _ctrl{_fc};
    int _showMode = 0; // 0=full, 1=head, 2=body, 3=arms, 4=legs

    Demo(const Options& o) : Application(o) {
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        std::cout << "Building humanoid mesh..." << std::endl;
        _mesh = _builder.buildMesh();
        std::cout << "Done. Bones: " << _builder.skeleton().boneCount() << std::endl;
        // Don't play animation - just keep skeleton in its bind pose
        _fc.target = glm::vec3(0, 0.85f, 0); _fc.dist = 4; _fc.pitch = 10;
    }

    ~Demo(){ImGui_ImplOpenGL3_Shutdown();ImGui_ImplGlfw_Shutdown();ImGui::DestroyContext();}

    void handleInput() override {
        _ctrl.begin(_input);
        _ctrl.update();
    }

    void renderFrame() override {
        glClearColor(0.10f,0.12f,0.16f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);glDisable(GL_CULL_FACE);

        auto proj=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.1f,50.f);
        auto view=_fc.viewMatrix();
        auto model=glm::mat4(1);

        // ALL bone matrices = identity (bind pose should produce I anyway)
        glm::mat4 bones[20] = {};
        for(int i=0;i<20;i++) bones[i]=glm::mat4(1);

        _shader.use();
        _shader.setMatrices(proj, view, model);
        _shader.setBoneMatrices(bones, 20);
        _shader.setColor(glm::vec3(0.4f,0.6f,0.85f));
        _shader.setLight(glm::vec3(0.5f,1,0.3f),glm::vec3(1,0.95f,0.85f),glm::vec3(3,4,-2),glm::vec3(0.3f,0.5f,1),5);
        _shader.setAmbient(0.35f);

        // Stats
        static bool first=true;
        if(first){std::cout<<"Vertices:"<<_mesh->indexCount()/3<<" triangles"<<std::endl;first=false;}

        _mesh->draw();

        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("Debug",nullptr,ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("Vertices: %d tris", _mesh->indexCount()/3);
        ImGui::Text("Bones: %d", _builder.skeleton().boneCount());
        ImGui::Text("All bone matrices = Identity");
        ImGui::Text("If you see a humanoid: mesh is OK");
        ImGui::Text("If only sphere+cylinder: mesh generation broken");
        ImGui::End();
        ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

int main(){Options o;o.windowTitle="09 Humanoid Debug";o.assetRootDir="media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
