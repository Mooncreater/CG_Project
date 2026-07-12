// 03: Materials & Textures
#include "../src/base/application.h"
#include "../src/orbit_control/orbit_control.h"
#include "../src/primitives/primitives.h"
#include "../src/mesh/mesh.h"
#include "../src/base/texture2d.h"
#include "../src/base/glsl_program.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

struct Demo : Application {
    std::unique_ptr<GLSLProgram> _s;
    std::unique_ptr<Mesh> _sphere, _cube, _cyl;
    std::unique_ptr<ImageTexture2D> _tex;

    FreeCamera _fc;
    OrbitControl _ctrl{ _fc };

    glm::vec3 _amb = { 0.2f,0.2f,0.2f }, _dif = { 0.6f,0.5f,0.4f }, _spc = { 0.3f,0.3f,0.3f };

    float _shi = 32, _rot = 0;
    int _shape = 0;


    Demo(const Options& o) :Application(o) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(_window, true);
        ImGui_ImplOpenGL3_Init("#version 330");

        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aP;
layout(location=1) in vec3 aN;
layout(location=2) in vec2 aT;

out vec3 fn,fp;
out vec2 fu;
uniform mat4 uM,uV,uP;

void main(){
    vec4 w = uM*vec4(aP,1);
    fp = w.xyz;
    fn = normalize(mat3(transpose(inverse(uM))) * aN);
    fu = aT;
    gl_Position = uP * uV * w;
})";

        const char* fs = R"(
#version 330 core
in vec3 fn,fp,fu2;
in vec2 fu;
out vec4 o;
uniform sampler2D uTex;
uniform bool uUseTex;

uniform vec3 uAmb,uDif,uSpc,uLPos,uVPos;
uniform float uShi;

void main(){
    vec3 col = uUseTex ? texture(uTex,fu).rgb : uDif;
    vec3 N = normalize(fn);
    vec3 L = normalize(uLPos-fp);
    vec3 V = normalize(uVPos-fp);
    vec3 H = normalize(L+V);

    o = vec4(col * uAmb + col * max(dot(N, L), 0.) + uSpc * pow(max(dot(N, H), 0.), uShi), 1);
})";

        _s = std::make_unique<GLSLProgram>();
        _s->attachVertexShader(vs);
        _s->attachFragmentShader(fs);
        _s->link();

        auto m = [](auto& e) {return std::make_unique<Mesh>(e.vertices, e.indices);
            };

        _sphere = m(Primitives::CreateSphere(1.2f, 48));
        _cube = m(Primitives::CreateCube(2));
        _cyl = m(Primitives::CreateCylinder(1, 2.5f, 48));

        try {
            _tex = std::make_unique<ImageTexture2D>(_assetRootDir + "/texture/miscellaneous/Rock-Texture-Surface.jpg");
        }
        catch (...) {}
        _fc.target = glm::vec3(0, 0, 0);
        _fc.dist = 7;
        _fc.pitch = 20;

    }
    void handleInput() override {
        _ctrl.begin(_input);
        _ctrl.update();
    }
    void renderFrame() override {
        glClearColor(0.1f, 0.12f, 0.15f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        _rot += _deltaTime * 0.3f;

        auto P = glm::perspective(glm::radians(50.f), (float)_windowWidth / _windowHeight, 0.5f, 50.f);

        auto V = _fc.viewMatrix();

        auto M = glm::rotate(glm::mat4(1), _rot, glm::vec3(0, 1, 0));

        _s->use();
        _s->setUniformMat4("uP", P);
        _s->setUniformMat4("uV", V);

        _s->setUniformVec3("uAmb", _amb);
        _s->setUniformVec3("uDif", _dif);
        _s->setUniformVec3("uSpc", _spc);

        _s->setUniformFloat("uShi", _shi);
        _s->setUniformVec3("uLPos", glm::vec3(5, 8, 5));
        _s->setUniformVec3("uVPos", _fc.position());

        bool t = _tex != nullptr;
        _s->setUniformBool("uUseTex", t);
        if (t)_tex->bind(0);

        auto draw = [&](Mesh* m, float ox, float oz) {
            _s->setUniformMat4("uM", glm::translate(M, glm::vec3(ox, 0, oz)));
            m->draw();
            };

        if (_shape == 0)draw(_sphere.get(), 0, 0);
        else if (_shape == 1)draw(_cube.get(), 0, 0);

        else if (_shape == 2)draw(_cyl.get(), 0, 0);
        else {
            draw(_sphere.get(), -2, -2);
            draw(_cube.get(), 2, -2);
            draw(_cyl.get(), 0, 2);
        }
        glDisable(GL_DEPTH_TEST);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Material");
        ImGui::Text("Shape:");
        const char* sh[] = { "Sphere","Cube","Cylinder","All" };

        for (int i = 0; i < 4; i++) {
            if (ImGui::Selectable(sh[i], _shape == i)) _shape = i;
            if (i < 3) ImGui::SameLine();
        }
        ImGui::ColorEdit3("Ambient", &_amb[0]);
        ImGui::ColorEdit3("Diffuse", &_dif[0]);

        ImGui::ColorEdit3("Specular", &_spc[0]);
        ImGui::SliderFloat("Shininess", &_shi, 1, 256);

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};

int main() {
    Options o;
    o.windowTitle = "03 Materials";
    o.assetRootDir = "media";
    o.windowWidth = 1024;
    o.windowHeight = 768;
    o.glVersion = { 3,3 };

    Demo app(o);
    app.run();
}
