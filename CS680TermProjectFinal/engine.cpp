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
        m_deltaTime = deltaTime;

        ProcessInput(deltaTime);
        Display(m_window->getWindow(), currentFrame, deltaTime);  // pass BOTH
        glfwPollEvents();
    }
    m_running = false;
}

//void Engine::ProcessInput(float deltaTime)
//{
//    GLFWwindow* win = m_window->getWindow();
//    Camera* cam = m_graphics->getCamera();
//
//    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
//        glfwSetWindowShouldClose(win, true);
//
//    // Ship controls for exploration mode
//    if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
//        m_graphics->setKeyState(Graphics::FWD, true);
//    else
//        m_graphics->setKeyState(Graphics::FWD, false);
//    if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
//        m_graphics->setKeyState(Graphics::BACK, true);
//    else
//        m_graphics->setKeyState(Graphics::BACK, false);
//    if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
//        m_graphics->setKeyState(Graphics::ROLL_L, true);
//    else
//        m_graphics->setKeyState(Graphics::ROLL_L, false);
//    if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
//        m_graphics->setKeyState(Graphics::ROLL_R, true);
//    else
//        m_graphics->setKeyState(Graphics::ROLL_R, false);
//
//    // Update view/projection matrices after any input
//    cam->Update();
//}

void Engine::ProcessInput(float deltaTime)
{
    GLFWwindow* win = m_window->getWindow();

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // Mode toggle with TAB (edge-triggered so one press = one toggle)
    bool tabNow = glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabNow && !m_tabWasPressed) {
        m_gameMode = (m_gameMode == EXPLORATION) ? PLANETARY : EXPLORATION;
    }
    m_tabWasPressed = tabNow;

    m_graphics->setKeyState(Graphics::FWD, glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
    m_graphics->setKeyState(Graphics::BACK, glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
    m_graphics->setKeyState(Graphics::ROLL_L, glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS);
    m_graphics->setKeyState(Graphics::ROLL_R, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
    m_graphics->setKeyState(Graphics::PITCH_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
    m_graphics->setKeyState(Graphics::PITCH_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);

    m_graphics->getCamera()->Update();
}

void Engine::cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (!s_instance) return;

    float fx = (float)xpos;
    float fy = (float)ypos;

    if (s_instance->m_firstMouse) {
        s_instance->m_lastX = fx;
        s_instance->m_lastY = fy;
        s_instance->m_firstMouse = false;
        return;
    }

    float xoffset = fx - s_instance->m_lastX;
    float yoffset = s_instance->m_lastY - fy;  // inverted once only, no double-negate

    s_instance->m_lastX = fx;
    s_instance->m_lastY = fy;

    s_instance->m_graphics->setMouseDelta(xoffset, yoffset);  // no negation here
}

void Engine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!s_instance) return;
    Camera* cam = s_instance->m_graphics->getCamera();
    cam->ProcessMouseScroll((float)yoffset);
    cam->Update();
}

void Engine::Display(GLFWwindow* window, double absoluteTime, float deltaTime)
{
    m_graphics->HierarchicalUpdate2(absoluteTime, deltaTime);  // split time params

    //m_graphics->getCamera()->SetThirdPerson(
    //    m_graphics->getShipPosition(),
    //    m_graphics->getShipForward(),
    //    m_graphics->getShipUp(),
    //    m_graphics->getShipRight()
    //);

    if (m_gameMode == EXPLORATION) {
        m_graphics->getCamera()->SetThirdPersonSmooth(
            m_graphics->getShipPosition(),
            m_graphics->getShipForward(),
            m_graphics->getShipUp(),
            deltaTime
        );
    }

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