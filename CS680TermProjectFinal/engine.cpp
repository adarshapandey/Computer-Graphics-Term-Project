// engine.cpp
// Top-level game loop: owns the window, graphics subsystem, and input processing.
// Three game modes — Exploration (ship flight), Planetary (orbit camera), Cockpit
// (first-person from ship nose) — are toggled with TAB and V.

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

void Engine::ProcessInput(float deltaTime)
{
    GLFWwindow* win = m_window->getWindow();

    if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(win, true);

    // TAB: toggle Exploration <-> Planetary Observation (edge-triggered)
    bool tabNow = glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS;
    if (tabNow && !m_tabWasPressed) {
        if (m_gameMode != PLANETARY) {
            m_gameMode = PLANETARY;
            m_graphics->SetPlanetaryMode(true);
            m_orbitInitialized = false;
        }
        else {
            m_gameMode = EXPLORATION;
            m_graphics->SetPlanetaryMode(false);
        }
    }
    m_tabWasPressed = tabNow;

    // V: toggle Exploration <-> Cockpit first-person view (edge-triggered)
    bool vNow = glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS;
    if (vNow && !m_vWasPressed) {
        if (m_gameMode != COCKPIT) {
            m_gameMode = COCKPIT;
            m_graphics->SetPlanetaryMode(false);
            m_cockpitInitialized = false;  // snap camera on first Display() tick
        }
        else {
            m_gameMode = EXPLORATION;
        }
    }
    m_vWasPressed = vNow;

    if (m_gameMode == EXPLORATION) {
        // Ship flight controls — active only in Exploration mode
        m_graphics->setKeyState(Graphics::FWD, glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::BACK, glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::LEFT, glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::RIGHT, glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::ROLL_L, glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::ROLL_R, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::PITCH_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
        m_graphics->setKeyState(Graphics::PITCH_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);
    }
    else {
        // Freeze ship in all non-Exploration modes
        for (int k = Graphics::FWD; k <= Graphics::PITCH_DOWN; ++k)
            m_graphics->setKeyState(k, false);

        if (m_gameMode == PLANETARY) {
            // N = next body, B = previous body (edge-triggered)
            bool nNow = glfwGetKey(win, GLFW_KEY_N) == GLFW_PRESS;
            if (nNow && !m_nWasPressed) {
                m_graphics->CycleBody(1);
                m_graphics->getCamera()->SetOrbitRadius(m_graphics->GetSelectedBodyOrbitRadius());
            }
            m_nWasPressed = nNow;

            bool bNow = glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS;
            if (bNow && !m_bWasPressed) {
                m_graphics->CycleBody(-1);
                m_graphics->getCamera()->SetOrbitRadius(m_graphics->GetSelectedBodyOrbitRadius());
            }
            m_bWasPressed = bNow;

            // R = reset orbit view (restore default azimuth/elevation)
            bool rNow = glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS;
            if (rNow && !m_rWasPressed) {
                m_graphics->getCamera()->ResetOrbit(
                    m_graphics->GetSelectedBodyPos(),
                    m_graphics->GetSelectedBodyOrbitRadius()
                );
            }
            m_rWasPressed = rNow;
        }
    }

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

    // Accumulate; Display() will route to ship or orbit camera based on mode
    s_instance->m_pendingMouseDX += xoffset;
    s_instance->m_pendingMouseDY += yoffset;
}

void Engine::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (!s_instance) return;
    Camera* cam = s_instance->m_graphics->getCamera();
    if (s_instance->m_gameMode == PLANETARY) {
        cam->OrbitZoom((float)yoffset);          // zoom into/away from planet
    }
    else {
        cam->ProcessMouseScroll((float)yoffset); // FoV zoom for Exploration & Cockpit
        cam->Update();
    }
}

void Engine::Display(GLFWwindow* window, double absoluteTime, float deltaTime)
{
    // Update solar system and ship physics
    m_graphics->HierarchicalUpdate2(absoluteTime, deltaTime);

    Camera* cam = m_graphics->getCamera();

    if (m_gameMode == EXPLORATION) {
        // Route mouse delta to ship steering
        m_graphics->setMouseDelta(m_pendingMouseDX, m_pendingMouseDY);

        // Smooth third-person camera follows ship from behind
        cam->SetThirdPersonSmooth(
            m_graphics->getShipPosition(),
            m_graphics->getShipForward(),
            m_graphics->getShipUp(),
            deltaTime
        );

    }
    else if (m_gameMode == PLANETARY) {
        // Orbit camera around the selected celestial body

        if (!m_orbitInitialized) {
            cam->ResetOrbit(
                m_graphics->GetSelectedBodyPos(),
                m_graphics->GetSelectedBodyOrbitRadius()
            );
            m_orbitInitialized = true;
        }

        // Keep orbit target locked onto the planet as it moves
        cam->MoveOrbitTarget(m_graphics->GetSelectedBodyPos());

        // Mouse rotates the orbit view (Y inverted for natural feel)
        cam->UpdateOrbit(m_pendingMouseDX, -m_pendingMouseDY);

    }
    else { // COCKPIT — ship-front first-person view (V key)
        // Camera placed at the nose of the ship — 1.5 units ahead along the ship's
        // forward so the ship body stays completely behind the camera.
        // Only snaps position+orientation on entry; mouse drives free-look after.
        if (!m_cockpitInitialized) {
            glm::vec3 nosePos = m_graphics->getShipPosition()
                + m_graphics->getShipForward() * 1.5f;
            cam->SetCockpitEntry(nosePos,
                m_graphics->getShipForward(),
                m_graphics->getShipUp());
            m_cockpitInitialized = true;
        }
        // Mouse = free-look (look anywhere from the ship's nose position)
        cam->ProcessMouseMovement(m_pendingMouseDX, m_pendingMouseDY);
        cam->Update();
    }

    // Consume mouse delta — used by whichever mode was active
    m_pendingMouseDX = 0.f;
    m_pendingMouseDY = 0.f;

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