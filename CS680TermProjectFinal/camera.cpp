#include "camera.h"

Camera::Camera() {}
Camera::~Camera() {}

bool Camera::Initialize(int w, int h)
//--Init the view and projection matrices
//  if you will be having a moving camera the view matrix will need to more dynamic
//  ...Like you should update it before you render more dynamic 
//  for this project having them static will be fine
//view = glm::lookAt( glm::vec3(x, y, z), //Eye Position
//                    glm::vec3(0.0, 0.0, 0.0), //Focus point
//                    glm::vec3(0.0, 1.0, 0.0)); //Positive Y is up
{
    m_windowW = w;
    m_windowH = h;

    // Build initial front vector from euler angles
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    Update();
    return true;
}

void Camera::Update()
{
    // Rebuild view from current pos/front/up
    view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

    // Rebuild projection in case FoV changed (scroll zoom)
    projection = glm::perspective(
        glm::radians(fov),
        float(m_windowW) / float(m_windowH),
        0.01f,
        300.f);
}

glm::mat4 Camera::GetProjection() { return projection; }
glm::mat4 Camera::GetView() { return view; }

void Camera::ProcessKeyboard(int direction, float deltaTime)
{
    float velocity = moveSpeed * deltaTime;

    // Forward/backward move along the ground-projected front vector
    // (so W/S don't fly the camera up/down)
    glm::vec3 flatFront = glm::normalize(
        glm::vec3(cameraFront.x, 0.f, cameraFront.z));
    glm::vec3 right = glm::normalize(glm::cross(flatFront, cameraUp));

    if (direction == FORWARD)   cameraPos += flatFront * velocity;
    if (direction == BACKWARD)  cameraPos -= flatFront * velocity;
    if (direction == LEFT)      cameraPos -= right * velocity;
    if (direction == RIGHT)     cameraPos += right * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Clamp pitch so camera doesn't flip
    if (pitch > 89.f) pitch = 89.f;
    if (pitch < -89.f) pitch = -89.f;

    // Rebuild front vector from updated euler angles
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void Camera::ProcessMouseScroll(float yoffset)
{
    fov -= yoffset;
    if (fov < 5.f)  fov = 5.f;
    if (fov > 90.f)  fov = 90.f;
}

void Camera::SetThirdPerson(glm::vec3 shipPos, glm::vec3 shipForward, glm::vec3 shipUp, glm::vec3 shipRight)
{
    float followDist = 5.0f;
    float followUp = 1.5f;
    cameraPos = shipPos - shipForward * followDist + shipUp * followUp;
    //cameraFront = shipForward;
    cameraFront = glm::normalize((shipPos + shipForward * 2.0f) - cameraPos); // look slightly ahead of ship
    cameraUp = shipUp;
    Update();
}

void Camera::SetThirdPersonSmooth(glm::vec3 shipPos, glm::vec3 shipForward,
    glm::vec3 shipUp, float dt)
{
    float followDist = 5.0f;
    float followUp = 1.5f;
    float lerpSpeed = 5.0f;   // how fast camera catches up (lower = more lag)

    // Where the camera WANTS to be
    glm::vec3 desiredPos = shipPos - shipForward * followDist + shipUp * followUp;

    // Smoothly move camera toward desired position
    cameraPos = glm::mix(cameraPos, desiredPos, lerpSpeed * dt);

    // Always look at a point slightly ahead of the ship
    glm::vec3 lookTarget = shipPos + shipForward * 2.0f;
    cameraFront = glm::normalize(glm::mix(cameraFront, glm::normalize(lookTarget - cameraPos), lerpSpeed * dt));

    cameraUp = shipUp;
    Update();
}