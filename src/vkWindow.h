#pragma once

#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

class vkRenderer;

class vkWindow
{
public:

    vkWindow(vkRenderer* r) : m_vkRenderer(r)
    {
    }

    ~vkWindow()
    {
        glfwDestroyWindow(m_GLFWwindow);
        glfwTerminate();
    }

    void initGLFW();

    inline bool isOpen() const
    {
        return !glfwWindowShouldClose(m_GLFWwindow);
    }
    inline void pollEvents()
    {
        glfwPollEvents();
    }

private:

    GLFWwindow* m_GLFWwindow = nullptr;
    vkRenderer* m_vkRenderer = nullptr;
    VkExtent2D m_WindowSize = { 1024, 1024 };

    // GLFW callback functions
    static void onWindowResized(GLFWwindow* window, int width, int height);
    static void onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void onCursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void onMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};