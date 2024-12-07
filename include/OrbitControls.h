
#pragma once

#include "glm/glm.hpp"

struct Camera;
struct OrbitControls
{
    double yaw = 0.0;
    double pitch = 0.0;
    double radius = 2;
    glm::dvec3 target = {0.0, 0.0, 0.0};
    glm::dmat4 world;

    bool isLeftDown = false;
    bool isRightDown = false;

    glm::dvec2 mousePos;
    Camera *camera;

    OrbitControls();

    void setCamera(Camera *camera);

    glm::dvec3 getDirection();
    glm::dvec3 getPosition();
    glm::dmat4 getRotation();

    void translate_local(double x, double y, double z);

    void onMouseButton(int button, int action, int mods);
    void onMouseMove(double xpos, double ypos);
    void onMouseScroll(double xoffset, double yoffset);

    void update();
};