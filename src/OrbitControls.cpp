
#include "OrbitControls.h"
#include <Camera.h>

OrbitControls::OrbitControls()
{
}

void OrbitControls::setCamera(Camera *camera)
{
    this->camera = camera;
}

glm::dvec3 OrbitControls::getDirection()
{
    auto rotation = getRotation();

    auto dir = rotation * glm::dvec4(0, 1, 0, 1.0);

    return dir;
}

glm::dvec3 OrbitControls::getPosition()
{
    auto dir = getDirection();

    auto pos = target - (radius * dir);

    return pos;
}

glm::dmat4 OrbitControls::getRotation()
{
    glm::dvec3 up = {0, 0, 1};
    glm::dvec3 right = {1, 0, 0};

    auto rotYaw = glm::rotate(yaw, up);
    auto rotPitch = glm::rotate(pitch, right);

    // auto rotation = rotYaw;
    // auto rotation = rotPitch * rotYaw;
    auto rotation = rotPitch * rotYaw;

    return rotation;
}

void OrbitControls::translate_local(double x, double y, double z)
{
    auto _pos = glm::dvec3(0, 0, 0);
    auto _right = glm::dvec3(1, 0, 0);
    auto _forward = glm::dvec3(0, 1, 0);
    auto _up = glm::dvec3(0, 0, 1);

    _pos = world * glm::dvec4(_pos, 1);
    _right = world * glm::dvec4(_right, 1);
    _forward = world * glm::dvec4(_forward, 1);
    _up = world * glm::dvec4(_up, 1);

    _right = glm::normalize(_right - _pos) * x;
    _forward = glm::normalize(_forward - _pos) * y;
    _up = glm::normalize(_up - _pos) * (-z);

    this->target = this->target + +_right + _forward + _up;
}

void OrbitControls::onMouseButton(int button, int action, int mods)
{

    if (button == 0 && action == 1)
    {
        isLeftDown = true;
    }
    else if (action == 0)
    {
        isLeftDown = false;
    }

    if (button == 1 && action == 1)
    {
        isRightDown = true;
    }
    else if (action == 0)
    {
        isRightDown = false;
    }
}

void OrbitControls::onMouseScroll(double xoffset, double yoffset)
{

    // +1: zoom in
    // -1: zoom out

    if (yoffset < 0.0)
    {
        radius = radius * 1.1;
    }
    else
    {
        radius = radius / 1.1;
    }
}

void OrbitControls::update()
{
    glm::dvec3 up = {0, 0, 1};
    glm::dvec3 right = {1, 0, 0};

    auto translateRadius = glm::translate(glm::dmat4(), glm::dvec3(0.0, 0.0, radius));
    auto translateTarget = glm::translate(glm::dmat4(), target);
    auto rotYaw = glm::rotate(yaw, up);
    auto rotPitch = glm::rotate(pitch, right);

    auto flip = glm::dmat4(1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, -1.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.0);

    world = translateTarget * rotYaw * rotPitch * flip * translateRadius;
    camera->world = world;
    camera->position = world * glm::dvec4(0.0, 0.0, 0.0, 1.0);
    camera->update();
}

void OrbitControls::onMouseMove(double xpos, double ypos)
{
    glm::dvec2 newMousePos = {xpos, ypos};
    glm::dvec2 diff = newMousePos - mousePos;

    if (isLeftDown)
    {
        yaw -= double(diff.x) / 400.0;
        pitch -= double(diff.y) / 400.0;
    }
    else if (isRightDown)
    {
        auto ux = diff.x / 1000.0;
        auto uy = diff.y / 1000.0;

        translate_local(-ux * radius, uy * radius, 0);
    }

    mousePos = newMousePos;
}