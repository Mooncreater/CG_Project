#include "../src/base/application.h"
#include "../src/base/framebuffer.h"
#include "../src/base/glsl_program.h"
#include "../src/base/texture2d.h"
#include "../src/mesh/mesh.h"

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>

namespace {

constexpr float kCameraNear = 0.05f;
constexpr float kCameraFar = 100.0f;

struct FlyCamera {
    glm::vec3 position{0.0f, 1.6f, 5.5f};
    float yaw = -90.0f;
    float pitch = 0.0f;
    float fov = 60.0f;

    glm::vec3 forward() const {
        const float yawRad = glm::radians(yaw);
        const float pitchRad = glm::radians(pitch);
        return glm::normalize(glm::vec3(
            std::cos(yawRad) * std::cos(pitchRad),
            std::sin(pitchRad),
            std::sin(yawRad) * std::cos(pitchRad)));
    }

    glm::vec3 right() const {
        return glm::normalize(glm::cross(forward(), glm::vec3(0.0f, 1.0f, 0.0f)));
    }

    glm::mat4 view() const {
        return glm::lookAt(position, position + forward(), glm::vec3(0.0f, 1.0f, 0.0f));
    }

    glm::mat4 transform() const {
        const glm::vec3 f = forward();
        const glm::vec3 r = right();
        const glm::vec3 u = glm::normalize(glm::cross(r, f));

        glm::mat4 m(1.0f);
        m[0] = glm::vec4(r, 0.0f);
        m[1] = glm::vec4(u, 0.0f);
        m[2] = glm::vec4(-f, 0.0f);
        m[3] = glm::vec4(position, 1.0f);
        return m;
    }

    static FlyCamera fromTransform(const glm::mat4& transform, float fovDegrees) {
        FlyCamera result;
        result.position = glm::vec3(transform[3]);

        const glm::vec3 forwardVector = glm::normalize(-glm::vec3(transform[2]));
        result.pitch = glm::degrees(std::asin(glm::clamp(forwardVector.y, -1.0f, 1.0f)));
        result.yaw = glm::degrees(std::atan2(forwardVector.z, forwardVector.x));
        result.fov = fovDegrees;
        return result;
    }
};

enum class World {
    First,
    Second
};

struct PortalCrossingState {
    glm::vec3 previousNearPlaneCenter{0.0f};
    bool initialized = false;
    float cooldown = 0.0f;
};

struct PortalRenderTarget {
    int width = 0;
    int height = 0;
    Framebuffer framebuffer;
    Texture2D color;
    Texture2D depth;

    PortalRenderTarget(int w, int h)
        : width(w),
          height(h),
          color(GL_RGBA16F, w, h, GL_RGBA, GL_FLOAT),
          depth(GL_DEPTH_COMPONENT32F, w, h, GL_DEPTH_COMPONENT, GL_FLOAT) {
        color.bind();
        color.setParamterInt(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        color.setParamterInt(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        color.setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        color.setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        color.unbind();

        depth.bind();
        depth.setParamterInt(GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        depth.setParamterInt(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        depth.setParamterInt(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        depth.setParamterInt(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        depth.unbind();

        framebuffer.bind();
        framebuffer.attachTexture2D(color, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
        framebuffer.attachTexture2D(depth, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D);
        framebuffer.drawBuffers({GL_COLOR_ATTACHMENT0});
        const GLenum status = framebuffer.checkStatus();
        framebuffer.unbind();

        if (status != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error(framebuffer.getDiagnostic(status));
        }
    }
};

struct SceneShader {
    GLSLProgram program;

    SceneShader() {
        static const char* kVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPosition;
out vec3 worldNormal;

void main() {
    vec4 wp = model * vec4(inPosition, 1.0);
    worldPosition = wp.xyz;
    worldNormal = normalize(transpose(inverse(mat3(model))) * inNormal);
    gl_Position = projection * view * wp;
}
)GLSL";

        static const char* kFragment = R"GLSL(
#version 330 core
layout(location = 0) out vec4 fragColor;

in vec3 worldPosition;
in vec3 worldNormal;

uniform vec3 baseColor;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform vec3 cameraPosition;
uniform float emissiveStrength;

void main() {
    vec3 n = normalize(worldNormal);
    vec3 l = normalize(-lightDirection);
    vec3 v = normalize(cameraPosition - worldPosition);
    vec3 h = normalize(l + v);

    float diffuse = max(dot(n, l), 0.0);
    float specular = pow(max(dot(n, h), 0.0), 48.0);

    vec3 ambient = baseColor * 0.18;
    vec3 color = ambient + baseColor * lightColor * diffuse + lightColor * specular * 0.18;
    color += baseColor * emissiveStrength;

    fragColor = vec4(color, 1.0);
}
)GLSL";

        program.attachVertexShader(kVertex);
        program.attachFragmentShader(kFragment);
        program.link();
    }
};

struct PortalShader {
    GLSLProgram program;

    PortalShader() {
        static const char* kVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 texCoord;
out vec4 clipPosition;

void main() {
    vec4 wp = model * vec4(inPosition, 1.0);
    clipPosition = projection * view * wp;
    texCoord = inTexCoord;
    gl_Position = clipPosition;
}
)GLSL";

        static const char* kFragment = R"GLSL(
#version 330 core
layout(location = 0) out vec4 fragColor;

in vec2 texCoord;
in vec4 clipPosition;

uniform sampler2D portalTexture;
uniform float time;

void main() {
    vec2 uv = clipPosition.xy / clipPosition.w;
    uv = uv * 0.5 + 0.5;

    vec2 distortion = vec2(
        sin(uv.y * 30.0 + time * 0.8),
        cos(uv.x * 28.0 + time * 0.7)) * 0.0008;

    vec3 portalColor = texture(portalTexture, uv + distortion).rgb;
    vec2 border = min(texCoord, 1.0 - texCoord);
    float edgeDistance = min(border.x, border.y);
    float edgeGlow = 1.0 - smoothstep(0.0, 0.045, edgeDistance);

    vec3 glowColor = vec3(0.22, 0.42, 1.0);
    vec3 color = portalColor + glowColor * edgeGlow * 0.32;
    color = vec3(1.0) - exp(-color * 1.15);

    fragColor = vec4(color, 1.0);
}
)GLSL";

        program.attachVertexShader(kVertex);
        program.attachFragmentShader(kFragment);
        program.link();
    }
};

struct SkyShader {
    GLSLProgram program;

    SkyShader() {
        static const char* kVertex = R"GLSL(
#version 330 core
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 direction;

void main() {
    direction = normalize(inPosition);
    vec4 position = projection * mat4(mat3(view)) * model * vec4(inPosition, 1.0);
    gl_Position = position.xyww;
}
)GLSL";

        static const char* kFragment = R"GLSL(
#version 330 core
layout(location = 0) out vec4 fragColor;

in vec3 direction;

void main() {
    vec3 d = normalize(direction);
    float h = d.y * 0.5 + 0.5;

    vec3 horizon = vec3(1.15, 0.22, 0.10);
    vec3 middle = vec3(0.12, 0.05, 0.22);
    vec3 zenith = vec3(0.006, 0.012, 0.055);

    vec3 color = mix(horizon, middle, smoothstep(0.0, 0.30, h));
    color = mix(color, zenith, smoothstep(0.30, 0.92, h));
    fragColor = vec4(color, 1.0);
}
)GLSL";

        program.attachVertexShader(kVertex);
        program.attachFragmentShader(kFragment);
        program.link();
    }
};

glm::vec3 transformPoint(const glm::mat4& transform, const glm::vec3& point) {
    return glm::vec3(transform * glm::vec4(point, 1.0f));
}

glm::mat4 modelMatrix(
    const glm::vec3& position, const glm::vec3& scale, float rotationYDegrees = 0.0f) {
    glm::mat4 result(1.0f);
    result = glm::translate(result, position);
    result = glm::rotate(result, glm::radians(rotationYDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
    result = glm::scale(result, scale);
    return result;
}

glm::vec3 getNearPlaneCenter(const FlyCamera& camera, float nearDistance) {
    return camera.position + camera.forward() * nearDistance;
}

glm::mat4 calculatePortalCameraTransform(
    const glm::mat4& mainCameraTransform,
    const glm::mat4& sourceDoorTransform,
    const glm::mat4& targetDoorTransform) {
    const glm::mat4 localCamera = glm::inverse(sourceDoorTransform) * mainCameraTransform;
    const glm::mat4 flip =
        glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
    return targetDoorTransform * flip * localCamera;
}

std::unique_ptr<Mesh> makeCubeMesh() {
    static const std::array<float, 36 * 8> kVertices = {
        -1,-1,-1,  0, 0,-1,  0,0,  1, 1,-1,  0, 0,-1,  1,1,  1,-1,-1,  0, 0,-1,  1,0,
         1, 1,-1,  0, 0,-1,  1,1, -1,-1,-1,  0, 0,-1,  0,0, -1, 1,-1,  0, 0,-1,  0,1,
        -1,-1, 1,  0, 0, 1,  0,0,  1,-1, 1,  0, 0, 1,  1,0,  1, 1, 1,  0, 0, 1,  1,1,
         1, 1, 1,  0, 0, 1,  1,1, -1, 1, 1,  0, 0, 1,  0,1, -1,-1, 1,  0, 0, 1,  0,0,
        -1, 1, 1, -1, 0, 0,  1,0, -1, 1,-1, -1, 0, 0,  1,1, -1,-1,-1, -1, 0, 0,  0,1,
        -1,-1,-1, -1, 0, 0,  0,1, -1,-1, 1, -1, 0, 0,  0,0, -1, 1, 1, -1, 0, 0,  1,0,
         1, 1, 1,  1, 0, 0,  1,0,  1,-1,-1,  1, 0, 0,  0,1,  1, 1,-1,  1, 0, 0,  1,1,
         1,-1,-1,  1, 0, 0,  0,1,  1, 1, 1,  1, 0, 0,  1,0,  1,-1, 1,  1, 0, 0,  0,0,
        -1,-1,-1,  0,-1, 0,  0,1,  1,-1,-1,  0,-1, 0,  1,1,  1,-1, 1,  0,-1, 0,  1,0,
         1,-1, 1,  0,-1, 0,  1,0, -1,-1, 1,  0,-1, 0,  0,0, -1,-1,-1,  0,-1, 0,  0,1,
        -1, 1,-1,  0, 1, 0,  0,1,  1, 1, 1,  0, 1, 0,  1,0,  1, 1,-1,  0, 1, 0,  1,1,
         1, 1, 1,  0, 1, 0,  1,0, -1, 1,-1,  0, 1, 0,  0,1, -1, 1, 1,  0, 1, 0,  0,0
    };

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    vertices.reserve(36);
    indices.reserve(36);

    for (uint32_t i = 0; i < 36; ++i) {
        const std::size_t base = static_cast<std::size_t>(i) * 8;
        vertices.push_back(Vertex(
            glm::vec3(kVertices[base], kVertices[base + 1], kVertices[base + 2]),
            glm::vec3(kVertices[base + 3], kVertices[base + 4], kVertices[base + 5]),
            glm::vec2(kVertices[base + 6], kVertices[base + 7])));
        indices.push_back(i);
    }

    return std::make_unique<Mesh>(vertices, indices);
}

std::unique_ptr<Mesh> makeQuadMesh() {
    std::vector<Vertex> vertices = {
        Vertex(glm::vec3(-1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f)),
        Vertex(glm::vec3( 1.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)),
        Vertex(glm::vec3( 1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)),
        Vertex(glm::vec3(-1.0f,  1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f))
    };
    std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0};
    return std::make_unique<Mesh>(vertices, indices);
}

class PortalDemo : public Application {
public:
    PortalDemo(const Options& options)
        : Application(options),
          _cube(makeCubeMesh()),
          _quad(makeQuadMesh()),
          _portalTarget(std::make_unique<PortalRenderTarget>(_windowWidth, _windowHeight)) {
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        _portalShader.program.use();
        _portalShader.program.setUniformInt("portalTexture", 0);
    }

protected:
    void handleInput() override {
        handleMouseLook();

        if (_input.keyboard.keyStates[GLFW_KEY_ESCAPE] == GLFW_PRESS) {
            glfwSetWindowShouldClose(_window, GLFW_TRUE);
        }

        const float speed = 3.5f * _deltaTime;
        const glm::vec3 forward = _camera.forward();
        const glm::vec3 right = _camera.right();

        if (_input.keyboard.keyStates[GLFW_KEY_W] != GLFW_RELEASE) _camera.position += forward * speed;
        if (_input.keyboard.keyStates[GLFW_KEY_S] != GLFW_RELEASE) _camera.position -= forward * speed;
        if (_input.keyboard.keyStates[GLFW_KEY_A] != GLFW_RELEASE) _camera.position -= right * speed;
        if (_input.keyboard.keyStates[GLFW_KEY_D] != GLFW_RELEASE) _camera.position += right * speed;
        if (_input.keyboard.keyStates[GLFW_KEY_Q] != GLFW_RELEASE) _camera.position.y -= speed;
        if (_input.keyboard.keyStates[GLFW_KEY_E] != GLFW_RELEASE) _camera.position.y += speed;
    }

    void renderFrame() override {
        if (_windowReized || _portalTarget->width != _windowWidth || _portalTarget->height != _windowHeight) {
            _portalTarget = std::make_unique<PortalRenderTarget>(_windowWidth, _windowHeight);
            _windowReized = false;
        }

        const bool crossed = tryCrossPortal();
        if (crossed) {
            glfwSetWindowTitle(
                _window,
                _currentWorld == World::First ? "10 Portal - First World" : "10 Portal - Second World");
        }

        const glm::mat4& activeDoor = _currentWorld == World::First ? _sourceDoorTransform : _targetDoorTransform;
        const glm::mat4& remoteDoor = _currentWorld == World::First ? _targetDoorTransform : _sourceDoorTransform;

        const glm::mat4 portalCameraTransform =
            calculatePortalCameraTransform(_camera.transform(), activeDoor, remoteDoor);
        const FlyCamera portalCamera = FlyCamera::fromTransform(portalCameraTransform, _camera.fov);

        const glm::mat4 portalProjection = glm::perspective(
            glm::radians(portalCamera.fov),
            static_cast<float>(_portalTarget->width) / static_cast<float>(_portalTarget->height),
            kCameraNear,
            kCameraFar);

        _portalTarget->framebuffer.bind();
        glViewport(0, 0, _portalTarget->width, _portalTarget->height);
        glClearColor(0.006f, 0.008f, 0.025f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (_currentWorld == World::First) {
            renderPortalWorld(portalCamera, portalProjection);
        } else {
            renderRealWorld(portalCamera, portalProjection);
        }

        _portalTarget->framebuffer.unbind();

        glViewport(0, 0, _windowWidth, _windowHeight);
        glClearColor(0.07f, 0.08f, 0.09f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const glm::mat4 mainProjection = glm::perspective(
            glm::radians(_camera.fov),
            static_cast<float>(_windowWidth) / static_cast<float>(_windowHeight),
            kCameraNear,
            kCameraFar);

        if (_currentWorld == World::First) {
            renderRealWorld(_camera, mainProjection);
        } else {
            renderPortalWorld(_camera, mainProjection);
        }

        renderDoorFrame(activeDoor, mainProjection);
        renderPortalSurface(activeDoor, mainProjection);

        showFpsInWindowTitle();
        _input.forwardState();
    }

private:
    SceneShader _sceneShader;
    PortalShader _portalShader;
    SkyShader _skyShader;

    std::unique_ptr<Mesh> _cube;
    std::unique_ptr<Mesh> _quad;
    std::unique_ptr<PortalRenderTarget> _portalTarget;

    FlyCamera _camera;
    bool _firstMouse = true;
    double _lastMouseX = 0.0;
    double _lastMouseY = 0.0;

    World _currentWorld = World::First;
    PortalCrossingState _crossingState;

    const glm::mat4 _sourceDoorTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, 0.0f));
    const glm::mat4 _targetDoorTransform =
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, -4.0f));

    float currentTime() const {
        return static_cast<float>(glfwGetTime());
    }

    void handleMouseLook() {
        if (!_input.mouse.press.right) {
            _firstMouse = true;
            return;
        }

        if (_firstMouse) {
            _lastMouseX = _input.mouse.move.xNow;
            _lastMouseY = _input.mouse.move.yNow;
            _firstMouse = false;
        }

        const double dx = _input.mouse.move.xNow - _lastMouseX;
        const double dy = _lastMouseY - _input.mouse.move.yNow;
        _lastMouseX = _input.mouse.move.xNow;
        _lastMouseY = _input.mouse.move.yNow;

        constexpr float kSensitivity = 0.12f;
        _camera.yaw += static_cast<float>(dx) * kSensitivity;
        _camera.pitch += static_cast<float>(dy) * kSensitivity;
        _camera.pitch = glm::clamp(_camera.pitch, -89.0f, 89.0f);
    }

    bool tryCrossPortal() {
        _crossingState.cooldown = std::max(0.0f, _crossingState.cooldown - _deltaTime);

        const glm::vec3 currentNearPlaneCenter = getNearPlaneCenter(_camera, kCameraNear);
        if (!_crossingState.initialized) {
            _crossingState.previousNearPlaneCenter = currentNearPlaneCenter;
            _crossingState.initialized = true;
            return false;
        }

        const glm::mat4& activeDoor = _currentWorld == World::First ? _sourceDoorTransform : _targetDoorTransform;
        const glm::mat4& destinationDoor =
            _currentWorld == World::First ? _targetDoorTransform : _sourceDoorTransform;
        const glm::mat4 worldToDoor = glm::inverse(activeDoor);

        const glm::vec3 previousLocal =
            transformPoint(worldToDoor, _crossingState.previousNearPlaneCenter);
        const glm::vec3 currentLocal = transformPoint(worldToDoor, currentNearPlaneCenter);

        const bool crossedFromAllowedSide = previousLocal.z > 0.0f && currentLocal.z <= 0.0f;
        bool crossed = false;

        if (_crossingState.cooldown <= 0.0f && crossedFromAllowedSide) {
            const float denominator = previousLocal.z - currentLocal.z;
            if (std::abs(denominator) > 1.0e-6f) {
                const float t = glm::clamp(previousLocal.z / denominator, 0.0f, 1.0f);
                const glm::vec3 crossingLocal = glm::mix(previousLocal, currentLocal, t);

                constexpr float kDoorHalfWidth = 1.0f;
                constexpr float kDoorHalfHeight = 1.5f;

                const bool insideDoor = std::abs(crossingLocal.x) <= kDoorHalfWidth &&
                    std::abs(crossingLocal.y) <= kDoorHalfHeight;

                if (insideDoor) {
                    const glm::mat4 flip =
                        glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
                    const glm::mat4 transfer = destinationDoor * flip * glm::inverse(activeDoor);

                    _camera = FlyCamera::fromTransform(transfer * _camera.transform(), _camera.fov);
                    _currentWorld = _currentWorld == World::First ? World::Second : World::First;
                    _crossingState.previousNearPlaneCenter = getNearPlaneCenter(_camera, kCameraNear);
                    _crossingState.cooldown = 0.35f;
                    crossed = true;
                }
            }
        }

        if (!crossed) {
            _crossingState.previousNearPlaneCenter = currentNearPlaneCenter;
        }

        return crossed;
    }

    void setupSceneShader(
        const FlyCamera& camera,
        const glm::mat4& projection,
        const glm::vec3& lightDirection,
        const glm::vec3& lightColor) {
        _sceneShader.program.use();
        _sceneShader.program.setUniformMat4("view", camera.view());
        _sceneShader.program.setUniformMat4("projection", projection);
        _sceneShader.program.setUniformVec3("cameraPosition", camera.position);
        _sceneShader.program.setUniformVec3("lightDirection", glm::normalize(lightDirection));
        _sceneShader.program.setUniformVec3("lightColor", lightColor);
    }

    void drawSceneObject(const glm::mat4& model, const glm::vec3& color, float emissive = 0.0f) {
        _sceneShader.program.setUniformMat4("model", model);
        _sceneShader.program.setUniformVec3("baseColor", color);
        _sceneShader.program.setUniformFloat("emissiveStrength", emissive);
        _cube->draw();
    }

    void renderRealWorld(const FlyCamera& camera, const glm::mat4& projection) {
        setupSceneShader(
            camera,
            projection,
            glm::vec3(-0.5f, -1.0f, -0.25f),
            glm::vec3(1.0f, 0.94f, 0.84f));

        drawSceneObject(
            modelMatrix(glm::vec3(0.0f, -0.15f, 0.0f), glm::vec3(8.0f, 0.1f, 8.0f)),
            glm::vec3(0.23f, 0.24f, 0.22f));
        drawSceneObject(
            modelMatrix(glm::vec3(-3.0f, 1.0f, -2.5f), glm::vec3(0.8f, 1.0f, 0.8f)),
            glm::vec3(0.34f, 0.25f, 0.17f));
        drawSceneObject(
            modelMatrix(glm::vec3(3.2f, 0.6f, -1.0f), glm::vec3(0.7f, 0.6f, 0.7f)),
            glm::vec3(0.18f, 0.22f, 0.25f));
    }

    void renderPortalWorld(const FlyCamera& camera, const glm::mat4& projection) {
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        _skyShader.program.use();
        _skyShader.program.setUniformMat4("model", glm::scale(glm::mat4(1.0f), glm::vec3(30.0f)));
        _skyShader.program.setUniformMat4("view", camera.view());
        _skyShader.program.setUniformMat4("projection", projection);
        _cube->draw();
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);

        setupSceneShader(
            camera,
            projection,
            glm::vec3(-0.3f, -1.0f, -0.4f),
            glm::vec3(1.0f, 0.50f, 0.28f));

        drawSceneObject(
            modelMatrix(glm::vec3(0.0f, -0.35f, 4.0f), glm::vec3(12.0f, 0.1f, 12.0f)),
            glm::vec3(0.04f, 0.08f, 0.13f),
            0.15f);

        const float time = currentTime();
        for (int i = -3; i <= 3; ++i) {
            const float x = static_cast<float>(i) * 1.7f;
            const float z = 1.5f + std::abs(static_cast<float>(i)) * 0.6f;
            const float height = 0.5f + 0.25f * std::sin(time * 0.6f + static_cast<float>(i));

            drawSceneObject(
                modelMatrix(glm::vec3(x, height * 0.5f, z), glm::vec3(0.18f, height, 0.18f)),
                glm::vec3(0.20f, 0.33f, 0.62f),
                0.75f);
        }

        drawSceneObject(
            modelMatrix(glm::vec3(0.0f, 1.8f, 6.0f), glm::vec3(1.5f, 0.08f, 1.5f)),
            glm::vec3(1.0f, 0.28f, 0.08f),
            2.0f);
    }

    void renderDoorFrame(const glm::mat4& activeDoor, const glm::mat4& projection) {
        setupSceneShader(
            _camera,
            projection,
            glm::vec3(-0.5f, -1.0f, -0.25f),
            glm::vec3(1.0f, 0.94f, 0.84f));

        const glm::vec3 woodColor(0.31f, 0.17f, 0.08f);
        drawSceneObject(
            activeDoor * modelMatrix(glm::vec3(-1.08f, 0.0f, 0.0f), glm::vec3(0.08f, 1.6f, 0.12f)),
            woodColor);
        drawSceneObject(
            activeDoor * modelMatrix(glm::vec3(1.08f, 0.0f, 0.0f), glm::vec3(0.08f, 1.6f, 0.12f)),
            woodColor);
        drawSceneObject(
            activeDoor * modelMatrix(glm::vec3(0.0f, 1.58f, 0.0f), glm::vec3(1.16f, 0.08f, 0.12f)),
            woodColor);
    }

    void renderPortalSurface(const glm::mat4& activeDoor, const glm::mat4& projection) {
        const glm::vec3 cameraInDoorSpace = transformPoint(glm::inverse(activeDoor), _camera.position);
        if (cameraInDoorSpace.z <= 0.0f) {
            return;
        }

        glDisable(GL_CULL_FACE);
        _portalShader.program.use();
        _portalShader.program.setUniformMat4(
            "model",
            activeDoor * modelMatrix(glm::vec3(0.0f, 0.0f, -0.015f), glm::vec3(1.0f, 1.5f, 1.0f)));
        _portalShader.program.setUniformMat4("view", _camera.view());
        _portalShader.program.setUniformMat4("projection", projection);
        _portalShader.program.setUniformFloat("time", currentTime());

        _portalTarget->color.bind(0);
        _quad->draw();
        glEnable(GL_CULL_FACE);
    }
};

}  // namespace

int main() {
    Options options{};
    options.windowTitle = "10 Portal";
    options.assetRootDir = "media";
    options.windowWidth = 1280;
    options.windowHeight = 720;
    options.windowResizable = true;
    options.vSync = true;
    options.msaa = true;
    options.glVersion = {3, 3};
    options.backgroundColor = glm::vec4(0.07f, 0.08f, 0.09f, 1.0f);

    PortalDemo app(options);
    app.run();
    return 0;
}
