#ifndef ENGINE_H
#define ENGINE_H
#include <assert.h>
#include "window.h"
#include "graphics.h"

class Engine
{
public:
    Engine(const char* name, int width, int height);
    ~Engine();
    bool Initialize();
    void Run();
    void ProcessInput(float deltaTime);
    unsigned int getDT();
    long long GetCurrentTimeMillis();
    void Display(GLFWwindow*, double);

    // Static GLFW callbacks — need access to engine instance
    static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
    static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

    // Pointer to self so static callbacks can reach the camera
    static Engine* s_instance;

private:
    Window* m_window;
    const char* m_WINDOW_NAME;
    int          m_WINDOW_WIDTH;
    int          m_WINDOW_HEIGHT;
    bool         m_FULLSCREEN;
    Graphics* m_graphics;
    bool         m_running;

    // Mouse state
    float m_lastX = 400.f;
    float m_lastY = 300.f;
    bool  m_firstMouse = true;

    // Timing
    float m_lastFrame = 0.f;
};
#endif // ENGINE_H