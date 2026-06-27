// 01: Primitives — uses SimpleLitShader + OrbitControl
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
    std::unique_ptr<Mesh> _m[6]; int _sel=0; float _rot=0;
    const char* _names[6]={"Cube","Sphere","Cylinder","Cone","Prism","Frustum"};

    Demo(const Options& o):Application(o){
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        auto m=[](auto& e){return std::make_unique<Mesh>(e.vertices,e.indices);};
        _m[0]=m(Primitives::CreateCube(1.5f)); _m[1]=m(Primitives::CreateSphere(1,36));
        _m[2]=m(Primitives::CreateCylinder(1,2,36)); _m[3]=m(Primitives::CreateCone(1,2,36));
        _m[4]=m(Primitives::CreatePrism(6,1,2)); _m[5]=m(Primitives::CreateFrustum(6,0.6f,1.2f,2));
        _fc.target=glm::vec3(0,0,0);_fc.dist=8;_fc.pitch=20;
    }
    void handleInput() override { _ctrl.begin(_input); _ctrl.update(); }
    void renderFrame() override {
        glClearColor(0.15f,0.15f,0.2f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);_rot+=_deltaTime*0.5f;
        glm::mat4 P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.5f,50.f);
        glm::mat4 M=glm::rotate(glm::mat4(1),_rot,glm::vec3(0,1,0));
        drawLit(_s,_m[_sel].get(),M,glm::vec3(0.3f,0.6f,0.9f),P,_fc.viewMatrix());
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("Primitives"); for(int i=0;i<6;i++)if(ImGui::Selectable(_names[i],_sel==i))_sel=i; ImGui::End();
        ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
int main(){Options o;o.windowTitle="01 Primitives";o.assetRootDir="D:/.zju/Professional_courses/CG/final/media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
