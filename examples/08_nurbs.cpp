// 08: NURBS Surface Modeling — Cox-de Boor algorithm
#include "../src/base/application.h"
#include "../src/render_util/simple_lit.h"
#include "../src/orbit_control/orbit_control.h"
#include "../src/nurbs/nurbs.h"
#include "../src/primitives/primitives.h"
#include "../src/mesh/mesh.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Demo : Application {
    SimpleLitShader _s; FreeCamera _fc; OrbitControl _ctrl{_fc};
    std::unique_ptr<Mesh> _mesh, _cpSphere;
    NURBS::NURBSSurface _nurbs;
    bool _wire=false,_showCP=true; int _samples=40,_selI=2,_selJ=2,_n=5;

    Demo(const Options& o):Application(o){
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        auto e=Primitives::CreateSphere(1,16);_cpSphere=std::make_unique<Mesh>(e.vertices,e.indices);
        _nurbs=NURBS::NURBSSurface::createPatch();_n=(int)_nurbs.cp.size();rebuild();
        _fc.target=glm::vec3(0,0,0);_fc.dist=10;_fc.pitch=30;
    }
    void rebuild(){_nurbs.kU=NURBS::makeKnots(_nurbs.dU,_n);_nurbs.kV=NURBS::makeKnots(_nurbs.dV,_n);_mesh=std::make_unique<Mesh>(_nurbs.tessellate(_samples,_samples).vertices,_nurbs.tessellate(_samples,_samples).indices);}
    void handleInput() override { _ctrl.begin(_input); _ctrl.update(); }
    void renderFrame() override {
        glClearColor(0.10f,0.12f,0.18f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        auto P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.5f,50.f);auto V=_fc.viewMatrix();
        if(_wire){glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);drawLit(_s,_mesh.get(),glm::mat4(1),glm::vec3(0.3f,0.6f,0.9f),P,V);glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);}
        else drawLit(_s,_mesh.get(),glm::mat4(1),glm::vec3(0.3f,0.6f,0.9f),P,V);
        if(_showCP)for(int j=0;j<_n;j++)for(int i=0;i<_n;i++){
            bool sel=(i==_selI&&j==_selJ);glm::vec3 c=sel?glm::vec3(1,0.9f,0):glm::vec3(0.9f,0.3f,0.2f);
            drawLit(_s,_cpSphere.get(),glm::scale(glm::translate(glm::mat4(1),_nurbs.cp[j][i]),glm::vec3(.12f*(sel?1.8f:1))),c,P,V);
        }
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("NURBS");ImGui::Checkbox("Wireframe",&_wire);ImGui::SameLine();ImGui::Checkbox("CPs",&_showCP);
        ImGui::SliderInt("Samples",&_samples,4,80);ImGui::SliderInt("DegU",&_nurbs.dU,1,4);ImGui::SliderInt("DegV",&_nurbs.dV,1,4);
        if(ImGui::Button("Rebuild"))rebuild();
        ImGui::Separator();ImGui::Text("CP[%d][%d]",_selI,_selJ);
        ImGui::SliderInt("i",&_selI,0,_n-1);ImGui::SliderInt("j",&_selJ,0,_n-1);
        if(_selI>=0&&_selI<_n&&_selJ>=0&&_selJ<_n){
            auto&cp=_nurbs.cp[_selJ][_selI];float t[3]={cp.x,cp.y,cp.z};
            if(ImGui::SliderFloat3("Pos",t,-5,5)){cp=glm::vec3(t[0],t[1],t[2]);rebuild();}
            float w=_nurbs.w[_selJ][_selI];
            if(ImGui::SliderFloat("Weight",&w,0.1f,5)){_nurbs.w[_selJ][_selI]=w;rebuild();}
        }
        if(ImGui::Button("Reset")){_nurbs=NURBS::NURBSSurface::createPatch();_n=(int)_nurbs.cp.size();rebuild();}
        ImGui::End();ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
int main(){Options o;o.windowTitle="08 NURBS";o.assetRootDir="media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
