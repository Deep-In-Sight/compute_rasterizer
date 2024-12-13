#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

struct Camera;

class CameraController
{
  public:
    enum MouseButton
    {
        LEFT = 0,
        RIGHT = 1,
        NONE = 2
    };

    CameraController(Camera *camera = nullptr);
    ~CameraController();

    void setCamera(Camera *camera);

    void mousePressed(MouseButton button, float x, float y);
    void mouseMoved(float x, float y);
    void mouseReleased(MouseButton button);
    void mouseWheelRolled(float yangle);
    void touchPressed(int fingerId, float x, float y);
    void touchMoved(int fingerId, float dx, float dy);
    void touchReleased(int fingerId);

    glm::dvec3 getPosition();
    void setPosition(glm::dvec3 position);
    glm::dquat getQuaternion();
    void setQuaternion(glm::dquat quaternion);
    glm::dvec3 getEuler();
    void setEuler(glm::dvec3 euler);
    void setLookAt(glm::dvec3 target);
    // rotate speed, translate speed, zoom speed
    void setSpeed(glm::dvec3 speed);
    glm::dvec3 getSpeed();
    void lockYaw(bool enable);
    void setTopView(glm::dvec3 center, float distance);
    void setCenterView(glm::dvec3 center);

    // add translate to current position in local frame
    void translateLocal(glm::dvec3 translation);
    // yaw, pitch, roll
    void rotateLocal(glm::dvec3 yawpitchroll);

    void reset();
    virtual void update();

    Camera *camera;

  protected:
    bool m_moving;
    bool m_orienting;
    float m_rSpeed;
    float m_tSpeed;
    float m_zSpeed;
    bool m_yawLocked;
    glm::ivec2 m_lastMousePosition;
    glm::ivec2 m_fingerIds;
    glm::dvec2 m_fingerPositions[2];
    float m_lastPinchDistance;
    glm::dvec2 m_lastPinchPosition;
    glm::dvec3 m_position;
    glm::dquat m_quaternion;
};