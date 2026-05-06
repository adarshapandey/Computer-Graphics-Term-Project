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

    // Cockpit (first-person) view — position at ship front, look along ship forward
    void SetCockpitEntry(glm::vec3 pos, glm::vec3 forward, glm::vec3 up);

    // Orbit camera (Planetary Observation mode)
    void ResetOrbit(glm::vec3 target, float radius);   // first-time setup, resets angles
    void MoveOrbitTarget(glm::vec3 target);            // track moving planet each frame
    void UpdateOrbit(float dAzimuth, float dElevation);// mouse-driven rotation
    void OrbitZoom(float delta);                       // scroll-driven zoom
    void SetOrbitRadius(float r);                      // set radius directly (on planet cycle)

    // Directions
    enum MoveDir { FORWARD, BACKWARD, LEFT, RIGHT };

private:
    // Orbit camera state
    glm::vec3 m_orbitTarget    = glm::vec3(0.f);
    float     m_orbitRadius    = 5.f;
    float     m_orbitAzimuth   = 0.f;
    float     m_orbitElevation = 20.f;

    void ApplyOrbit();
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