// 07: Animation + Screenshot
#include "../src/base/application.h"
#include "../src/render_util/simple_lit.h"
#include "../src/orbit_control/orbit_control.h"
#include "../src/obj_loader/obj_loader.h"
#include "../src/primitives/primitives.h"
#include "../src/mesh/mesh.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

struct Demo : Application {
    SimpleLitShader _s; FreeCamera _fc; OrbitControl _ctrl{_fc};
    std::vector<std::unique_ptr<Mesh>> _frames;
    int _cur=0,_total=0;float _anim=0,_speed=1;bool _play=true,_loop=true;std::string _status;

    Demo(const Options& o):Application(o){
        IMGUI_CHECKVERSION();ImGui::CreateContext();ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(_window,true);ImGui_ImplOpenGL3_Init("#version 330");
        const char*files[]={"bunny.obj","dragon.obj","lucy.obj","knot.obj","rock.obj","arrow.obj"};
        for(auto&f:files)try{auto e=ObjLoader::Load(_assetRootDir+"/obj/"+f);_frames.push_back(std::make_unique<Mesh>(e.vertices,e.indices));}catch(...){}
        auto m=[](auto&e){return std::make_unique<Mesh>(e.vertices,e.indices);};
        if(_frames.empty()){_frames.push_back(m(Primitives::CreateCube(1)));_frames.push_back(m(Primitives::CreateSphere(1,32)));_frames.push_back(m(Primitives::CreateCylinder(1,1.5f,32)));_frames.push_back(m(Primitives::CreateCone(1,1.5f,32)));}
        _total=(int)_frames.size();_fc.target=glm::vec3(0,0,0);_fc.dist=5;_fc.pitch=15;
    }
    void saveSS(){
        std::vector<unsigned char> px(_windowWidth*_windowHeight*3);
        glReadPixels(0,0,_windowWidth,_windowHeight,GL_RGB,GL_UNSIGNED_BYTE,px.data());
        std::vector<unsigned char> fl(_windowWidth*_windowHeight*3);
        for(int y=0;y<_windowHeight;y++)memcpy(&fl[y*_windowWidth*3],&px[(_windowHeight-1-y)*_windowWidth*3],_windowWidth*3);
        char nm[64];snprintf(nm,sizeof(nm),"screenshot_%d.png",(int)time(0));
        stbi_write_png(nm,_windowWidth,_windowHeight,3,fl.data(),_windowWidth*3);_status="Saved: "+std::string(nm);
    }
    void handleInput() override {
        _ctrl.begin(_input);_ctrl.update();
        if(_input.keyboard.keyStates[GLFW_KEY_SPACE]==GLFW_PRESS)_play=!_play;
        if(_input.keyboard.keyStates[GLFW_KEY_RIGHT]==GLFW_PRESS){_cur=(_cur+1)%_total;_play=false;}
        if(_input.keyboard.keyStates[GLFW_KEY_LEFT]==GLFW_PRESS){_cur=(_cur-1+_total)%_total;_play=false;}
    }
    void renderFrame() override {
        glClearColor(0.10f,0.12f,0.16f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
        if(_play){_anim+=_deltaTime*_speed;_cur=((int)(_anim*5))%_total;if(!_loop&&_cur==_total-1)_play=false;}
        auto P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.1f,50.f);
        if(_cur>=0&&_cur<_total)drawLit(_s,_frames[_cur].get(),glm::mat4(1),glm::vec3(0.4f,0.6f,0.85f),P,_fc.viewMatrix());
        glDisable(GL_DEPTH_TEST);
        ImGui_ImplOpenGL3_NewFrame();ImGui_ImplGlfw_NewFrame();ImGui::NewFrame();
        ImGui::Begin("Animation");ImGui::Text("Frame:%d/%d",_cur+1,_total);
        if(ImGui::Button(_play?"Pause":"Play"))_play=!_play;ImGui::SameLine();
        if(ImGui::Button("Stop")){_play=false;_anim=0;_cur=0;}ImGui::SameLine();
        ImGui::Checkbox("Loop",&_loop);ImGui::SliderFloat("Speed",&_speed,0.1f,5);ImGui::SliderInt("Frame",&_cur,0,_total-1);
        if(ImGui::Button("Save Screenshot"))saveSS();
        if(!_status.empty())ImGui::TextColored(ImVec4(0.5f,1,0.5f,1),"%s",_status.c_str());
        ImGui::End();ImGui::Render();ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};
int main(){Options o;o.windowTitle="07 Animation";o.assetRootDir="D:/.zju/Professional_courses/CG/final/media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
