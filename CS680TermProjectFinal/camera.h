#ifndef CAMERA_H
#define CAMERA_H
#include "graphics_headers.h"

class Camera
{
public:
    Camera();
    ~Camera();
    bool Initialize(int w, int h);
    glm::mat4 GetProjection();
    glm::mat4 GetView();
    void Update();

    // Called by engine for input
    void ProcessKeyboard(int direction, float deltaTime);
    void ProcessMouseMovement(float xoffset, float yoffset);
    void ProcessMouseScroll(float yoffset);

    void SetThirdPerson(glm::vec3 shipPos, glm::vec3 shipForward, glm::vec3 shipUp, glm::vec3 shipRight);
    void SetThirdPersonSmooth(glm::vec3 shipPos, glm::vec3 shipForward,
        glm::vec3 shipUp, float dt);

    // Directions
    enum MoveDir { FORWARD, BACKWARD, LEFT, RIGHT };

private:
    // Camera position/orientation
    glm::vec3 cameraPos = glm::vec3(0.f, 5.f, -20.f);
    glm::vec3 cameraFront = glm::vec3(0.f, 0.f, 1.f);  // points toward origin
    glm::vec3 cameraUp = glm::vec3(0.f, 1.f, 0.f);

    // Euler angles
    float yaw = 90.f;   // pointing toward +Z (toward scene)
    float pitch = -10.f;  // slight downward look

    // Zoom (FoV)
    float fov = 60.f;

    // Sensitivity / speed
    float moveSpeed = 8.f;
    float mouseSensitivity = 0.1f;

    int m_windowW = 800;
    int m_windowH = 600;

    glm::mat4 projection;
    glm::mat4 view;
    bool m_thirdPerson = true;
};
#endif /* CAMERA_H */