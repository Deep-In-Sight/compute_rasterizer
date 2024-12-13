#include <Camera.h>
#include <CameraController.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

CameraController::CameraController(Camera *camera) : camera(camera)
{
    reset();
}

CameraController::~CameraController() = default;

void CameraController::setCamera(Camera *camera)
{
    this->camera = camera;
    update();
}

void CameraController::reset()
{
    m_moving = false;
    m_orienting = false;
    m_rSpeed = 0.001;
    m_tSpeed = 0.01;
    m_zSpeed = 0.1;
    m_yawLocked = false;
    m_fingerIds = glm::ivec2(-1, -1);
    m_fingerPositions[0] = glm::dvec2(0, 0);
    m_fingerPositions[1] = glm::dvec2(0, 0);
    m_lastPinchDistance = 0;
    m_lastPinchPosition = glm::dvec2(0, 0);
    m_position = glm::dvec3(0, 0, 0);
    m_quaternion = glm::dquat(1, 0, 0, 0);
}

void CameraController::mousePressed(MouseButton button, float x, float y)
{
    m_orienting = (button == LEFT);
    m_moving = (button == RIGHT);
    m_lastMousePosition = glm::ivec2(x, y);
}

void CameraController::mouseReleased(MouseButton button)
{
    m_orienting = (button == LEFT);
    m_moving = (button == RIGHT);
}

void CameraController::mouseMoved(float x, float y)
{
    auto dx = x - m_lastMousePosition[0];
    auto dy = y - m_lastMousePosition[1];
    m_lastMousePosition = glm::ivec2(x, y);
    if (m_orienting)
    {
        auto pitch = -dx * m_rSpeed;
        auto roll = -dy * m_rSpeed;
        rotateLocal(glm::dvec3(0, pitch, roll));
    }
    if (m_moving)
    {
        translateLocal(glm::dvec3(-dx * m_tSpeed, dy * m_tSpeed, 0));
    }
}

void CameraController::mouseWheelRolled(float yangle)
{
    float dz = (yangle > 0) ? 1 : -1;
    translateLocal(glm::dvec3(0, 0, dz * m_zSpeed));
}

void CameraController::touchPressed(int fingerId, float x, float y)
{
    if (m_fingerIds[0] == -1)
    {
        m_fingerIds[0] = fingerId;
        m_fingerPositions[0] = glm::dvec2(x, y);
    }
    else if (m_fingerIds[1] == -1)
    {
        m_fingerIds[1] = fingerId;
        m_fingerPositions[1] = glm::dvec2(x, y);
        m_lastPinchDistance = glm::distance(m_fingerPositions[0], m_fingerPositions[2]);
        m_lastPinchPosition = (m_fingerPositions[0] + m_fingerPositions[2]) / 2.0;
    }
}

void CameraController::touchReleased(int fingerId)
{
    if (m_fingerIds[0] == fingerId)
    {
        m_fingerIds[0] = -1;
    }
    else if (m_fingerIds[1] == fingerId)
    {
        m_fingerIds[1] = -1;
    }
}

void CameraController::touchMoved(int fingerId, float dx, float dy)
{
    // update the stored touchEvents
    if (m_fingerIds[0] == fingerId)
    {
        m_fingerPositions[0] += glm::dvec2(dx, dy);
    }
    else if (m_fingerIds[1] == fingerId)
    {
        m_fingerPositions[1] += glm::dvec2(dx, dy);
    }

    // single touch
    if (m_fingerIds[0] != -1 && m_fingerIds[1] == -1)
    {
        rotateLocal(glm::dvec3(-dx * m_rSpeed, -dy * m_rSpeed, 0));
    }

    // two finger touch
    if (m_fingerIds[0] != -1 && m_fingerIds[1] != -1)
    {
        glm::dvec2 p1 = m_fingerPositions[0];
        glm::dvec2 p2 = m_fingerPositions[1];
        auto d = glm::distance(p1, p2);
        auto p = (p1 + p2) / 2.0;
        auto dz = (m_lastPinchDistance - d);
        auto dxdy = (m_lastPinchPosition - p);
        m_lastPinchDistance = d;
        m_lastPinchPosition = p;
        translateLocal(glm::dvec3(-dxdy[0] * m_tSpeed, -dxdy[1] * m_tSpeed, dz * m_zSpeed));
    }
}

void CameraController::lockYaw(bool enable)
{
    m_yawLocked = enable;
}

void CameraController::translateLocal(glm::dvec3 translation)
{
    m_position += m_quaternion * translation;
    update();
}

static glm::dquat eulerToQuaternion(glm::dvec3 euler)
{
    glm::dquat yaw = glm::angleAxis(euler[0], glm::dvec3(0.0, 0.0, 1.0));
    glm::dquat pitch = glm::angleAxis(euler[1], glm::dvec3(0.0, 1.0, 0.0));
    glm::dquat roll = glm::angleAxis(euler[2], glm::dvec3(1.0, 0.0, 0.0));
    glm::dquat q = yaw * pitch * roll;
    return glm::normalize(q);
}

void CameraController::rotateLocal(glm::dvec3 angles)
{
    auto q = eulerToQuaternion(angles);
    m_quaternion = glm::normalize(m_quaternion * q);
    update();
}

void CameraController::setPosition(glm::dvec3 position)
{
    m_position = position;
    update();
}

glm::dvec3 CameraController::getPosition()
{
    return m_position;
}

void CameraController::setQuaternion(glm::dquat quaternion)
{
    m_quaternion = quaternion;
    update();
}

glm::dquat CameraController::getQuaternion()
{
    return m_quaternion;
}

void CameraController::setEuler(glm::dvec3 euler)
{
    m_quaternion = eulerToQuaternion(euler);
    update();
}

glm::dvec3 CameraController::getEuler()
{
    return glm::eulerAngles(m_quaternion);
}

void CameraController::setLookAt(glm::dvec3 target)
{
    glm::dvec3 forward = glm::normalize(target - m_position);
    glm::dvec3 backward = -forward;
    glm::dvec3 worldUp = glm::dvec3(0, 1, 0);
    glm::dvec3 right = glm::normalize(glm::cross(worldUp, backward));
    glm::dvec3 up = glm::normalize(glm::cross(backward, right));
    glm::dmat3 rot(right, up, backward);

    m_quaternion = glm::dquat(rot);
    update();
}

void CameraController::setSpeed(glm::dvec3 speed)
{
    m_rSpeed = speed[0];
    m_tSpeed = speed[1];
    m_zSpeed = speed[2];
}

glm::dvec3 CameraController::getSpeed()
{
    return glm::dvec3(m_rSpeed, m_tSpeed, m_zSpeed);
}

void CameraController::setTopView(glm::dvec3 center, float distance)
{
    // Position the camera
    glm::dvec3 cameraPosition = center + glm::dvec3(0, 0, distance);
    setPosition(cameraPosition);
    setLookAt(center);
}

void CameraController::setCenterView(glm::dvec3 center)
{
    setPosition(center);
    setEuler({0, 0, 3.14 / 2});
}

void CameraController::update()

{
    if (camera == nullptr)
    {
        return;
    }
    // construct transform matrix from m_position and m_quaternion
    glm::dmat4 world = glm::translate(glm::dmat4(), m_position);
    world *= glm::mat4_cast(m_quaternion);
    camera->world = world;
    camera->update();
}
