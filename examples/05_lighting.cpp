// 05: Lighting + Shadow — 3 switchable scenes
#include "../src/base/application.h"
#include "../src/base/glsl_program.h"
#include "../src/orbit_control/orbit_control.h"
#include "../src/primitives/primitives.h"
#include "../src/mesh/mesh.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glad/gl.h>

struct Demo : Application {
    std::unique_ptr<GLSLProgram> _shader, _shadowS;
    std::unique_ptr<Mesh> _sphere, _plane, _cube, _cyl, _cone;

    FreeCamera _fc;
    OrbitControl _ctrl{ _fc };

    GLuint _sfbo = 0, _smap = 0;
    int _ssz = 2048;

    bool _shadows = true;

    glm::vec3 _sd = { 0.5f,1,0.3f }, _sc = { 1,0.95f,0.85f };
    glm::vec3 _pp = { 3,4,-2 }, _pc = { 0.3f,0.5f,1 };

    float _pi = 5, _amb = 0.25f, _bias = 0.005f;
    int _sceneIdx = 0;

    const char* _sceneNames[3] = { "A:Indoor","B:Outdoor","C:Showcase" };


    Demo(const Options& o) :Application(o) {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(_window, true);
        ImGui_ImplOpenGL3_Init("#version 330");


        // Main shader
        const char* vs = R"(
#version 330 core
layout(location=0) in vec3 aP;
layout(location=1) in vec3 aN;

out vec3 fn, fp, flp;
uniform mat4 uM, uV, uP, uLVP;

void main(){
    vec4 w = uM * vec4(aP, 1);
    fp = w.xyz;
    fn = mat3(transpose(inverse(uM))) * aN;

    vec4 lp = uLVP * w;
    flp = lp.xyz / lp.w * 0.5 + 0.5;
    gl_Position = uP * uV * w;
})";

        const char* fs = R"(
#version 330 core
in vec3 fn, fp, flp;
out vec4 o;
uniform vec3 uSD, uSC, uPP, uPC, uOC;
uniform float uPI, uAmb, uBias;

uniform sampler2DShadow uShadow;
uniform bool uShadows;

float shadow(){
    if(!uShadows) return 1.0;
    float s=0;
    vec2 ts=1.0/textureSize(uShadow, 0);

    for(int x=-1; x<=1; x++)
        for(int y=-1; y<=1; y++)
            s+=texture(uShadow, vec3(flp.xy + vec2(x, y) * ts, flp.z - uBias));
    return s/9.0;
}
void main(){
    vec3 N = normalize(fn)；
    vec3 L = normalize(uSD);
    float nd = max(dot(N, L), 0.);
    float sh = shadow();

    vec3 tl = uPP-fp;
    float d = length(tl), att = uPI / (1 + d * d * .02);
    float np = max(dot(N, normalize(tl)), 0.);

    o = vec4(uOC * uAmb+uOC * uSC * nd * sh + uOC * uPC * np * att, 1);
}
)";

        _shader = std::make_unique<GLSLProgram>();
        _shader->attachVertexShader(vs);
        _shader->attachFragmentShader(fs);
        _shader->link();


        // Depth shader
        const char* dvs = R"(
#version 330 core
layout(location=0) in vec3 aP;

uniform mat4 uM, uLVP;

void main(){
    gl_Position = uLVP * uM * vec4(aP, 1);
}
)";

        const char* dfs = R"(
#version 330 core
void main(){
}
)";
        _shadowS = std::make_unique<GLSLProgram>();
        _shadowS->attachVertexShader(dvs);
        _shadowS->attachFragmentShader(dfs);
        _shadowS->link();

        // Shadow map
        glGenFramebuffers(1, &_sfbo);
        glGenTextures(1, &_smap);

        glBindTexture(GL_TEXTURE_2D, _smap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, _ssz, _ssz, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

        float bd[] = { 1,1,1,1 };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, bd);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

        glBindFramebuffer(GL_FRAMEBUFFER, _sfbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _smap, 0);

        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Meshes
        auto m = [](auto& e) {return std::make_unique<Mesh>(e.vertices, e.indices);
            };

        _sphere = m(Primitives::CreateSphere(0.5f, 36));
        _plane = m(Primitives::CreateCube(1));

        _cube = m(Primitives::CreateCube(1));
        _cyl = m(Primitives::CreateCylinder(0.6f, 2, 32));

        _cone = m(Primitives::CreateCone(0.7f, 1.5f, 32));


        _fc.target = glm::vec3(0, 1.5f, 0);
        _fc.dist = 10;
        _fc.pitch = 25;

    }

    ~Demo() {
        if (_sfbo)glDeleteFramebuffers(1, &_sfbo);
        if (_smap)glDeleteTextures(1, &_smap);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }

    void handleInput() override {
        _ctrl.begin(_input);
        _ctrl.update();
    }

    // ---- Shadow pass ----
    void shadowPass(glm::mat4 lVP) {
        glViewport(0, 0, _ssz, _ssz);
        glBindFramebuffer(GL_FRAMEBUFFER, _sfbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        glEnable(GL_DEPTH_TEST);
        glCullFace(GL_FRONT);

        _shadowS->use();
        _shadowS->setUniformMat4("uLVP", lVP);

        auto draw = [&](Mesh* m, glm::mat4 model) {_shadowS->setUniformMat4("uM", model);
        m->draw();
            };

        if (_sceneIdx == 0)drawSceneA_Shadow(draw);

        else if (_sceneIdx == 1)drawSceneB_Shadow(draw);

        else drawSceneC_Shadow(draw);

        glCullFace(GL_BACK);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glViewport(0, 0, _windowWidth, _windowHeight);

    }

    // ---- Scene A: Indoor room ----
    template<typename D> void drawSceneA_Shadow(D& draw) {
        draw(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -2, 0)), glm::vec3(10, 0.2f, 10)));
        //floor
        draw(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 1, -5)), glm::vec3(10, 6, 0.2f)));
        //back wall
        draw(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(-5, 1, 0)), glm::vec3(0.2f, 6, 10)));
        //left wall
        draw(_sphere.get(), glm::translate(glm::mat4(1), glm::vec3(-1.5f, -0.8f, -1)));
        //ball on floor
        draw(_cube.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(1.5f, -0.5f, -2)), glm::vec3(1.2f)));
        //box
    }

    template<typename D> void drawSceneA_Color(D& drawObj) {
        drawObj(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -2, 0)), glm::vec3(10, 0.2f, 10)), glm::vec3(0.6f, 0.55f, 0.45f));
        drawObj(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 1, -5)), glm::vec3(10, 6, 0.2f)), glm::vec3(0.7f, 0.65f, 0.55f));
        drawObj(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(-5, 1, 0)), glm::vec3(0.2f, 6, 10)), glm::vec3(0.65f, 0.6f, 0.5f));
        drawObj(_sphere.get(), glm::translate(glm::mat4(1), glm::vec3(-1.5f, -0.8f, -1)), glm::vec3(0.8f, 0.25f, 0.2f));
        drawObj(_cube.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(1.5f, -0.5f, -2)), glm::vec3(1.2f)), glm::vec3(0.2f, 0.45f, 0.7f));
    }

    // ---- Scene B: Outdoor field ----
    template<typename D> void drawSceneB_Shadow(D& draw) {
        draw(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -1.6f, 0)), glm::vec3(25, 0.2f, 25)));
        for (int x = -8; x <= 8; x += 4) {
            for (int z = -8; z <= 8; z += 4) {
                draw(_sphere.get(), glm::translate(glm::mat4(1), glm::vec3(x + 0.5f, -0.8f, z + 0.5f)));
            }
        }

        for (int x = -6; x <= 6; x += 6)
            draw(_cube.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(x, 0, 0)), glm::vec3(1.5f)));

        draw(_cyl.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0, -4)), glm::vec3(0.6f, 1, 0.6f)));
        draw(_cone.get(), glm::translate(glm::mat4(1), glm::vec3(0, 1.3f, -4)));
    }

    template<typename D> void drawSceneB_Color(D& drawObj) {
        drawObj(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -1.6f, 0)), glm::vec3(25, 0.2f, 25)), glm::vec3(0.3f, 0.55f, 0.25f));
        for (int x = -8; x <= 8; x += 4)
            for (int z = -8; z <= 8; z += 4)
                drawObj(_sphere.get(), glm::translate(glm::mat4(1), glm::vec3(x + 0.5f, -0.8f, z + 0.5f)), glm::vec3(0.3f + fabsf(x) * 0.04f, 0.5f, 0.6f));

        for (int x = -6; x <= 6; x += 6)
            drawObj(_cube.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(x, 0, 0)), glm::vec3(1.5f)), glm::vec3(0.8f, 0.3f, 0.2f));

        drawObj(_cyl.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0, -4)), glm::vec3(0.6f, 1, 0.6f)), glm::vec3(0.5f, 0.4f, 0.3f));
        drawObj(_cone.get(), glm::translate(glm::mat4(1), glm::vec3(0, 1.3f, -4)), glm::vec3(0.3f, 0.55f, 0.25f));
    }

    // ---- Scene C: Circle showcase ----
    template<typename D> void drawSceneC_Shadow(D& draw) {
        draw(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -1.5f, 0)), glm::vec3(14, 0.2f, 14)));
        for (int i = 0; i < 8; i++) {
            float a = i * 0.785f;
            draw(_sphere.get(), glm::translate(glm::mat4(1), glm::vec3(cos(a) * 3, 0, sin(a) * 3)));
        }

        draw(_cube.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0.5f, 0)), glm::vec3(1.5f)));
        draw(_cyl.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0.5f, 0)), glm::vec3(1.5f)));
    }

    template<typename D> void drawSceneC_Color(D& drawObj) {
        drawObj(_plane.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, -1.5f, 0)), glm::vec3(14, 0.2f, 14)), glm::vec3(0.35f, 0.4f, 0.45f));
        for (int i = 0; i < 8; i++) {
            float a = i * 0.785f;
            drawObj(_sphere.get(), glm::translate(glm::mat4(1), glm::vec3(cos(a) * 3, 0, sin(a) * 3)), glm::vec3(0.3f + 0.08f * i, 0.5f, 0.7f - 0.06f * i));
        }

        drawObj(_cube.get(), glm::scale(glm::translate(glm::mat4(1), glm::vec3(0, 0.5f, 0)), glm::vec3(1.5f)), glm::vec3(0.8f, 0.3f, 0.2f));
    }

    // ---- Main render ----
    void renderFrame() override {
        glm::vec3 L = glm::normalize(_sd);

        glm::mat4 lVP = glm::ortho(-14.f, 14.f, -14.f, 14.f, 1.f, 40.f) * glm::lookAt(L * 15.f, glm::vec3(0), glm::vec3(0, 1, 0));

        if (_shadows)shadowPass(lVP);


        glClearColor(0.08f, 0.1f, 0.14f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);

        auto P = glm::perspective(glm::radians(50.f), (float)_windowWidth / _windowHeight, 0.5f, 50.f);
        auto V = _fc.viewMatrix();

        _shader->use();
        _shader->setUniformMat4("uP", P);
        _shader->setUniformMat4("uV", V);
        _shader->setUniformMat4("uLVP", lVP);

        _shader->setUniformVec3("uSD", L);
        _shader->setUniformVec3("uSC", _sc);

        _shader->setUniformVec3("uPP", _pp);
        _shader->setUniformVec3("uPC", _pc);

        _shader->setUniformFloat("uPI", _pi);
        _shader->setUniformFloat("uAmb", _amb);
        _shader->setUniformFloat("uBias", _bias);

        _shader->setUniformBool("uShadows", _shadows);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, _smap);
        _shader->setUniformInt("uShadow", 1);


        auto drawObj = [&](Mesh* m, glm::mat4 model, glm::vec3 col) {
            _shader->setUniformMat4("uM", model);
            _shader->setUniformVec3("uOC", col);
            m->draw();
            };


        if (_sceneIdx == 0)drawSceneA_Color(drawObj);

        else if (_sceneIdx == 1)drawSceneB_Color(drawObj);

        else drawSceneC_Color(drawObj);


        // Point light marker
        _shader->setUniformBool("uShadows", false);

        drawObj(_sphere.get(), glm::scale(glm::translate(glm::mat4(1), _pp), glm::vec3(0.12f)), _pc);

        glDisable(GL_DEPTH_TEST);


        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Light+Shadow", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::Text("Scene:");

        for (int i = 0;
            i < 3;
            i++) {
            if (i > 0)ImGui::SameLine();
            ImGui::RadioButton(_sceneNames[i], &_sceneIdx, i);
        }
        ImGui::Separator();

        ImGui::Checkbox("Shadows", &_shadows);
        ImGui::SliderFloat("Bias", &_bias, 0.0001f, 0.05f, "%.4f");

        ImGui::SliderFloat("Ambient", &_amb, 0, 1);

        ImGui::Text("Sun Dir");
        ImGui::SliderFloat3("##sd", &_sd[0], -1, 1);
        ImGui::ColorEdit3("Sun Col", &_sc[0]);

        ImGui::Text("Point Pos");
        ImGui::SliderFloat3("##pp", &_pp[0], -5, 5);
        ImGui::ColorEdit3("Pt Col", &_pc[0]);
        ImGui::SliderFloat("Pt Int", &_pi, 0, 20);

        ImGui::End();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    }
};


int main() {
    Options o;
    o.windowTitle = "05 Lighting+Shadow";
    o.assetRootDir = "media";
    o.windowWidth = 1100;
    o.windowHeight = 800;
    o.glVersion = { 3,3 };

    Demo app(o);
    app.run();
}
