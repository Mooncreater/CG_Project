#include "render_flag.h"

RenderFlag::RenderFlag(const Options& options) : Application(options) {
    // create star shader
    const char* vsCode =
        "#version 330 core\n"
        "layout(location = 0) in vec2 aPosition;\n"
        "void main() {\n"
        "    gl_Position = vec4(aPosition, 0.0f, 1.0f);\n"
        "}\n";

    const char* fsCode =
        "#version 330 core\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    fragColor = vec4(1.0f, 0.870f, 0.0f, 1.0f);\n"
        "}\n";

    _starShader.reset(new GLSLProgram);
    _starShader->attachVertexShader(vsCode);
    _starShader->attachFragmentShader(fsCode);
    _starShader->link();

    // TODO: create 5 stars
    // hint: aspect_of_the_window = _windowWidth / _windowHeight
    // write your code here
    // ---------------------------------------------------------------
    // _stars[i].reset(new Star(ndc_position, rotation_in_radians, size_of_star,
    // aspect_of_the_window));、
    float aspect = (float)_windowWidth / _windowHeight;
    _stars[0].reset(new Star(glm::vec2(-0.70, 0.33), 1.0f * M_PI / 10.0f, 0.2f, aspect));
    _stars[1].reset(new Star(glm::vec2(-0.38f, 0.52f), 0, 0.06f, aspect));
    _stars[2].reset(new Star(glm::vec2(-0.25f, 0.41f), 4.0f*M_PI/15.0f, 0.06f, aspect));
    _stars[3].reset(new Star(glm::vec2(-0.25f, 0.23f), 8.0f*M_PI/15.0f, 0.06f, aspect));
    _stars[4].reset(new Star(glm::vec2(-0.38f, 0.11f), 12.0f*M_PI/15.0f, 0.06f, aspect));
    // ---------------------------------------------------------------
}

void RenderFlag::handleInput() {
    if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] != GLFW_RELEASE) {
        glfwSetWindowShouldClose(_window, true);
        return;
    }
}

void RenderFlag::renderFrame() {
    showFpsInWindowTitle();

    // we use background as the flag
    glClearColor(0.87f, 0.161f, 0.063f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    _starShader->use();
    for (int i = 0; i < 5; ++i) {
        if (_stars[i] != nullptr) {
            _stars[i]->draw();
        }
    }
}