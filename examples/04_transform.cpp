// 04: Transform
#include "../src/base/application.h"
#include "../src/render_util/simple_lit.h"
#include "../src/orbit_control/orbit_control.h"
#include "../src/primitives/primitives.h"
#include "../src/mesh/mesh.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Demo : Application {
    SimpleLitShader _s; FreeCamera _fc; OrbitControl _ctrl{_fc};
    std::unique_ptr<Mesh> _cube;
    glm::vec3 _t={0,0,0},_r={0,0,0},_sc={1,1,1};bool _auto=false;

    Demo(const Options& o):Application(o){
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        _cube=std::make_unique<Mesh>(Primitives::CreateCube(1).vertices,Primitives::CreateCube(1).indices);
        _fc.target=glm::vec3(0,0,0);_fc.dist=6;_fc.pitch=25;
    }
    void handleInput() override { _ctrl.begin(_input); _ctrl.update(); }
    void renderFrame() override {
        glClearColor(0.15f,0.15f,0.2f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);if(_auto)_r.y+=_deltaTime*1.5f;
        glm::mat4 P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.5f,50.f);
        glm::mat4 M=glm::translate(glm::mat4(1),_t);
        M=glm::rotate(M,_r.x,glm::vec3(1,0,0));M=glm::rotate(M,_r.y,glm::vec3(0,1,0));M=glm::rotate(M,_r.z,glm::vec3(0,0,1));
        M=glm::scale(M,_sc);
        drawLit(_s,_cube.get(),M,glm::vec3(0.3f,0.6f,0.85f),P,_fc.viewMatrix());
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("Transform");ImGui::SliderFloat3("Translate",&_t[0],-3,3);
        ImGui::SliderFloat3("Rotate",&_r[0],-6.28f,6.28f);ImGui::SliderFloat3("Scale",&_sc[0],0.1f,3);
        ImGui::Checkbox("Auto Rotate Y",&_auto);if(ImGui::Button("Reset")){_t=_r=glm::vec3(0);_sc=glm::vec3(1);}
        ImGui::End();ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
int main(){Options o;o.windowTitle="04 Transform";o.assetRootDir="media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
