#include "vkWindow.h"

void vkWindow::initGLFW()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_GLFWwindow = glfwCreateWindow(
        static_cast<int>(m_WindowSize.width),
        static_cast<int>(m_WindowSize.height),
        "pt",
        nullptr,
        nullptr);

    glfwSetWindowUserPointer(m_GLFWwindow, this);
    glfwSetKeyCallback(m_GLFWwindow, vkWindow::onKeyCallback);
    glfwSetWindowSizeCallback(m_GLFWwindow, vkWindow::onWindowResized);
    glfwSetMouseButtonCallback(m_GLFWwindow, vkWindow::onMouseButtonCallback);
    glfwSetCursorPosCallback(m_GLFWwindow, vkWindow::onCursorPositionCallback);
    glfwSetScrollCallback(m_GLFWwindow, vkWindow::onMouseScrollCallback);
}

void vkWindow::onWindowResized(GLFWwindow * window, int width, int height)
{
    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));

    // If dimensions are zero, window is propably minimized
    while (width == 0 || height == 0)
    {
        //glfwWaitEvents();
        return;
    }

    app->m_WindowSize = {
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
    //app->m_vkRenderer->windowResizeCallback(app->m_WindowSize);
}

void vkWindow::onKeyCallback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
}

void vkWindow::onCursorPositionCallback(GLFWwindow * window, double xpos, double ypos)
{
}

void vkWindow::onMouseButtonCallback(GLFWwindow * window, int button, int action, int mods)
{
}

void vkWindow::onMouseScrollCallback(GLFWwindow * window, double xoffset, double yoffset)
{
}
