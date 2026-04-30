#include "engine.h"

// Static instance pointer so GLFW callbacks can reach the camera
Engine* Engine::s_instance = nullptr;

Engine::Engine(const char* name, int width, int height)
{
    m_WINDOW_NAME = name;
    m_WINDOW_WIDTH = width;
    m_WINDOW_HEIGHT = height;
    s_instance = this;
}

Engine::~Engine()
{
    delete m_window;
    delete m_graphics;
    m_window = nullptr;
    m_graphics = nullptr;
}

bool Engine::Initialize()
{
    // Start window
    m_window = new Window(m_WINDOW_NAME, &m_WINDOW_WIDTH, &m_WINDOW_HEIGHT);
    if (!m_window->Initialize()) {
        printf("The window failed to initialize.\n");
        return false;
    }

    // Start graphics
    m_graphics = new Graphics();
    if (!m_graphics->Initialize(m_WINDOW_WIDTH, m_WINDOW_HEIGHT)) {
        printf("The graphics failed to initialize.\n");
        return false;
    }

    // Register GLFW callbacks
    glfwSetCursorPosCallback(m_window->getWindow(), cursor_position_callback);
    glfwSetScrollCallback(m_window->getWindow(), scroll_callback);

    // Capture mouse so it doesn't leave the window
    glfwSetInputMode(m_window->getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    return true;
}

void Engine::Run()
{
    m_running = true;
    m_lastFrame = (float)glfwGetTime();

    while (!glfwWindowShouldClose(m_window->getWindow()))
    {
        float currentFrame = (float)glfwGetTime();
        float deltaTime = currentFrame - m_lastFrame;
        m_lastFrame = currentFrame;

        ProcessInput(deltaTime);
        Display(m_window->getWindow(), currentFrame);
        glfwPollEvents();
    }
    m_running = false;
}

void Engine::ProcessInput(float deltaTime)
{
    GLFWwindow* win = m_window->getWindow();
    Camera* cam = m_graphics->getCamera();

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // WASD camera movement
    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
        cam->ProcessKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
        cam->ProcessKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
        cam->ProcessKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
        cam->ProcessKeyboard(Camera::RIGHT, deltaTime);

    // Update view/projection matrices after any input
    cam->Update();
}

void Engine::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!s_instance) return;
    Camera* cam = s_instance->m_graphics->getCamera();

    float fx = (float)xpos;
    float fy = (float)ypos;

    if (s_instance->m_firstMouse) {
        s_instance->m_lastX = fx;
        s_instance->m_lastY = fy;
        s_instance->m_firstMouse = false;
        return;
    }

    float xoffset = (fx - s_instance->m_lastX);  // right = positive yaw
    float yoffset = (s_instance->m_lastY - fy);  // up = positive pitch (Y flipped)

    s_instance->m_lastX = fx;
    s_instance->m_lastY = fy;

    cam->ProcessMouseMovement(xoffset, yoffset);
    cam->Update();
}

void Engine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!s_instance) return;
    Camera* cam = s_instance->m_graphics->getCamera();
    cam->ProcessMouseScroll((float)yoffset);
    cam->Update();
}

void Engine::Display(GLFWwindow* window, double time)
{
    m_graphics->HierarchicalUpdate2(time);
    m_graphics->Render();
    m_window->Swap();
}

unsigned int Engine::getDT()
{
    return (unsigned int)glfwGetTime();
}

long long Engine::GetCurrentTimeMillis()
{
    return (long long)glfwGetTime();
}