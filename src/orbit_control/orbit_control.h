#pragma once
#include "../camera/free_camera.h"
#include "../base/input.h"
#include <glm/glm.hpp>

class OrbitControl {
    FreeCamera& _cam;
    Input*     _input = nullptr;
    double     _prevMX = 0, _prevMY = 0;
    bool       _first  = true;

public:
    FreeCamera& cam() { return _cam; }
    OrbitControl(FreeCamera& cam) : _cam(cam) {}

    void begin(const Input& input) {
        _input = &const_cast<Input&>(input);
        if (_first) { _prevMX = _input->mouse.move.xNow; _prevMY = _input->mouse.move.yNow; _first = false; }
    }

    void orbit() {
        if (!_input) return;
        if (_input->mouse.press.right) {
            float dx = (float)(_input->mouse.move.xNow - _prevMX);
            float dy = (float)(_input->mouse.move.yNow - _prevMY);
            if (fabsf(dx) < 200 && fabsf(dy) < 200) _cam.orbit(dx * 0.3f, dy * 0.3f);
        }
        _prevMX = _input->mouse.move.xNow; _prevMY = _input->mouse.move.yNow;
    }

    void pan() {
        if (!_input) return;
        if (_input->mouse.press.middle) {
            float dx = (float)(_input->mouse.move.xNow - _prevMX);
            float dy = (float)(_input->mouse.move.yNow - _prevMY);
            if (fabsf(dx) < 200 && fabsf(dy) < 200) _cam.pan(dx, dy);
        }
    }

    void zoom() {
        if (!_input) return;
        float s = _input->mouse.scroll.yOffset;
        if (fabsf(s) < 0.01f) return;
        if (s >  5.0f) s =  5.0f;
        if (s < -5.0f) s = -5.0f;
        _cam.zoom(s);
    }

    void fly() {
        if (!_input) return;
        float sp = _cam.dist * 0.005f;
        if (_input->keyboard.keyStates[GLFW_KEY_W]==GLFW_PRESS) _cam.fly( 1,0,0,sp);
        if (_input->keyboard.keyStates[GLFW_KEY_S]==GLFW_PRESS) _cam.fly(-1,0,0,sp);
        if (_input->keyboard.keyStates[GLFW_KEY_A]==GLFW_PRESS) _cam.fly(0,-1,0,sp);
        if (_input->keyboard.keyStates[GLFW_KEY_D]==GLFW_PRESS) _cam.fly(0, 1,0,sp);
    }

    void update() { orbit(); zoom(); }
    void updateFull() { orbit(); pan(); zoom(); fly(); }
};
