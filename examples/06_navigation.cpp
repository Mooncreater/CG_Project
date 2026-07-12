// 06: Navigation (Zoom, Pan, Orbit, Zoom To Fit)
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
    std::unique_ptr<Mesh> _cube,_sphere,_cyl,_cone;
    std::vector<glm::vec3> _pos;

    Demo(const Options& o):Application(o){
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        auto m=[](auto&e){return std::make_unique<Mesh>(e.vertices,e.indices);};
        _cube=m(Primitives::CreateCube(1));_sphere=m(Primitives::CreateSphere(0.7f,32));
        _cyl=m(Primitives::CreateCylinder(0.6f,2,32));_cone=m(Primitives::CreateCone(0.7f,1.5f,32));
        _pos={{0,0,0},{-4,0,3},{3,0,-4},{5,0,2},{-3,0,-5},{0,0,6},{-6,0,-2}};
        _fc.target=glm::vec3(0,2,0);_fc.dist=18;_fc.pitch=25;
    }
    void handleInput() override {
        _ctrl.begin(_input);
        if(_input.keyboard.keyStates[GLFW_KEY_F]==GLFW_PRESS){glm::vec3 mn(1e9),mx(-1e9);
            for(auto&p:_pos){mn=glm::min(mn,p);mx=glm::max(mx,p);}
            _fc.target=(mn+mx)*0.5f;_fc.dist=glm::length(mx-mn)*1.2f;_fc.pitch=20;
        }
        _ctrl.updateFull();
    }
    void renderFrame() override {
        glClearColor(0.10f,0.12f,0.18f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
        auto P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.5f,100.f);auto V=_fc.viewMatrix();
        for(int i=-10;i<=10;i++){drawLit(_s,_cube.get(),glm::scale(glm::translate(glm::mat4(1),glm::vec3(i,0,0)),glm::vec3(0.02f,0.01f,20)),glm::vec3(0.3f),P,V);
            drawLit(_s,_cube.get(),glm::scale(glm::translate(glm::mat4(1),glm::vec3(0,0,i)),glm::vec3(20,0.01f,0.02f)),glm::vec3(0.3f),P,V);}
        Mesh*ms[]={_cube.get(),_sphere.get(),_cyl.get(),_cone.get()};
        glm::vec3 cs[]={{0.2f,0.5f,0.8f},{0.8f,0.3f,0.2f},{0.2f,0.7f,0.3f},{0.7f,0.4f,0.8f}};
        for(size_t i=0;i<_pos.size();i++)drawLit(_s,ms[i%4],glm::translate(glm::mat4(1),_pos[i]),cs[i%4],P,V);
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("Navigation");ImGui::Text("R-drag:Orbit M-drag:Pan Scroll:Zoom WASD:Move F:Fit");
        auto cp=_fc.position();ImGui::Text("Cam:%.0f,%.0f,%.0f Dist:%.0f",cp.x,cp.y,cp.z,_fc.dist);ImGui::End();
        ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
int main(){Options o;o.windowTitle="06 Navigation";o.assetRootDir="media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
