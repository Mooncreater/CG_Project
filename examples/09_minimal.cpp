// 09_minimal: minimal test — single colored cube
#include "../src/base/application.h"
#include "../src/render_util/simple_lit.h"
#include "../src/mesh/mesh.h"
#include "../src/orbit_control/orbit_control.h"

struct Demo : Application {
    std::unique_ptr<Mesh> _mesh;
    SimpleLitShader _shader;
    FreeCamera _fc;
    OrbitControl _ctrl{_fc};

    Demo(const Options& o) : Application(o) {
        std::vector<Vertex> verts;
        std::vector<uint32_t> idx;
        float s=0.5f;
        verts.push_back({{-s,-s,-s},{0,0,-1},{0,0}});
        verts.push_back({{ s,-s,-s},{0,0,-1},{0,0}});
        verts.push_back({{ s, s,-s},{0,0,-1},{0,0}});
        verts.push_back({{-s, s,-s},{0,0,-1},{0,0}});
        idx={0,1,2,0,2,3};
        _mesh=std::make_unique<Mesh>(verts,idx);
        _fc.target=glm::vec3(0,0,0);_fc.dist=3;
    }
    void handleInput()override{_ctrl.begin(_input);_ctrl.update();}
    void renderFrame()override{
        glClearColor(0.10f,0.12f,0.16f,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
        auto P=glm::perspective(glm::radians(50.f),(float)_windowWidth/_windowHeight,0.1f,50.f);
        drawLit(_shader,_mesh.get(),glm::mat4(1),glm::vec3(0.8f,0.3f,0.3f),P,_fc.viewMatrix());
    }
};
int main(){Options o;o.windowTitle="09 Minimal";o.assetRootDir="media";o.windowWidth=1024;o.windowHeight=768;o.glVersion={3,3};Demo app(o);app.run();return 0;}
