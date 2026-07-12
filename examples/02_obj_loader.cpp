// 02: OBJ Loader
#include "../src/base/application.h"
#include "../src/render_util/simple_lit.h"
#include "../src/orbit_control/orbit_control.h"
#include "../src/obj_loader/obj_loader.h"
#include "../src/primitives/primitives.h"
#include "../src/mesh/mesh.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Demo : Application {
    SimpleLitShader _s; FreeCamera _fc; OrbitControl _ctrl{_fc};
    std::unique_ptr<Mesh> _mesh; bool _wire=false; int _idx=0;
    std::vector<std::string> _files={"bunny.obj","dragon.obj","lucy.obj","knot.obj","rock.obj","arrow.obj"};

    Demo(const Options& o):Application(o){
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        load();_fc.target=glm::vec3(0,0,0);_fc.dist=5;_fc.pitch=15;
    }
    void load(){try{auto e=ObjLoader::Load(_assetRootDir+"/obj/"+_files[_idx]);_mesh=std::make_unique<Mesh>(e.vertices,e.indices);}catch(...){}}
    void handleInput() override { _ctrl.begin(_input); _ctrl.update(); }
    void renderFrame() override {
        glClearColor(0.12f,0.14f,0.18f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);if(_wire)glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        glm::mat4 P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.1f,50.f);
        if(_mesh)drawLit(_s,_mesh.get(),glm::mat4(1),glm::vec3(0.5f,0.6f,0.7f),P,_fc.viewMatrix());
        if(_wire)glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("OBJ");ImGui::Text("File: %s",_files[_idx].c_str());
        if(ImGui::Button("Next")){_idx=(_idx+1)%_files.size();load();}ImGui::SameLine();
        if(ImGui::Button("Prev")){_idx=(_idx-1+_files.size())%_files.size();load();}
        ImGui::Checkbox("Wireframe",&_wire);ImGui::End();
        ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
int main(){Options o;o.windowTitle="02 OBJ Loader";o.assetRootDir="media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
