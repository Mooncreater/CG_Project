#pragma once

#include <glm/glm.hpp>
#include <cmath>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class FreeCamera {
public:
    glm::vec3 target  = glm::vec3(0, 3, 0);
    float yaw   = 0.0f;
    float pitch = -30.0f;
    float dist  = 40.0f;

    void orbit(float dx, float dy) {
        yaw   += dx;
        pitch += dy;
        if (pitch >  89.0f) pitch =  89.0f;
        if (pitch < -89.0f) pitch = -89.0f;
    }

    void pan(float dx, float dy) {
        glm::vec3 front = forward();
        glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
        glm::vec3 up    = glm::cross(right, front);
        target += (-right * dx + up * dy) * (dist * 0.002f);
    }

    void zoom(float amount) {
        dist *= powf(0.997f, amount);
        if (dist <  1.0f) dist =  1.0f;
        if (dist > 50.0f) dist = 50.0f;
    }

    void zoomToFit(float sceneRadius) {
        // Fit the scene in view: distance = radius / tan(halfFov)
        dist = sceneRadius / tanf(glm::radians(30.0f));
        if (dist <  2.0f) dist =  2.0f;
        if (dist > 50.0f) dist = 50.0f;
        pitch = -30.0f;
    }
    void fly(float forwardAmt, float rightAmt, float upAmt, float speed) {
        glm::vec3 f = forward();
        glm::vec3 r = glm::normalize(glm::cross(f, glm::vec3(0, 1, 0)));
        target += (f * forwardAmt + r * rightAmt) * speed;
        target.y += upAmt * speed;
    }

    glm::vec3 position() const {
        float ry = glm::radians(yaw), rp = glm::radians(pitch);
        return target + glm::vec3(
            cos(ry) * cos(rp) * dist,
            sin(rp) * dist,
            sin(ry) * cos(rp) * dist);
    }

    glm::mat4 viewMatrix() const {
        return glm::lookAt(position(), target, glm::vec3(0, 1, 0));
    }

private:
    glm::vec3 forward() const {
        float ry = glm::radians(yaw), rp = glm::radians(pitch);
        return glm::normalize(glm::vec3(
            cos(ry) * cos(rp), sin(rp), sin(ry) * cos(rp)));
    }
};
