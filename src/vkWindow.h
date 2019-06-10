#pragma once

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "CameraControls.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <string>

class vkContext;

class vkWindow
{
    public:
    vkWindow(vkContext* r)
        : m_vkctx(r)
    {
    }

    ~vkWindow()
    {
        if(m_GLFWwindow != nullptr)
        {
            glfwDestroyWindow(m_GLFWwindow);
        }
        glfwTerminate();
    }

    void initGLFW();

    inline bool isOpen() const { return !glfwWindowShouldClose(m_GLFWwindow); }
    inline void pollEvents() { glfwPollEvents(); }
    GLFWwindow* getWindow() const { return m_GLFWwindow; }
    VkExtent2D  getWindowSize() const { return m_WindowSize; }
    void        update(float deltaTime);
    void        setWindowTitle(const std::string& title);

    CameraControls m_camera;

    private:
    GLFWwindow* m_GLFWwindow = nullptr;
    vkContext*  m_vkctx      = nullptr;
    VkExtent2D  m_WindowSize = {1280, 1280};

    void moveLightToCamera();

    struct
    {
        bool left = false, right = false, up = false, down = false, forward = false, back = false,
             l_shift = false;
    } keys;

    struct
    {
        glm::vec2 currentPosition;
        glm::vec2 lastPositon;
        glm::vec2 delta;
    } mouse;

    struct
    {
        bool left = false, middle = false, right = false;
    } mouseButtons;

    void handleKeyboardInput(int key, bool isPressed);
    void handleMouseButtonInput(int button, bool isPressed);
    void handleMouseCursorInput(double xpos, double ypos);
    void handleMouseScrollInput(double xpos, double ypos);


    // GLFW callback functions
    static void onErrorCallback(int error, const char* description);
    static void onWindowResized(GLFWwindow* window, int width, int height);
    static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onCursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void onMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};