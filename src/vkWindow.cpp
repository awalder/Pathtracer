#include "vkWindow.h"
#include <imgui_impl_glfw_vulkan.h>
#include <iostream>

#include "vkContext.h"

void vkWindow::initGLFW()
{
    glfwSetErrorCallback(onErrorCallback);
    glfwInit();

    if(!glfwVulkanSupported())
    {
        throw std::runtime_error("GLFW: Vulkan is not supported\n");
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    m_GLFWwindow =
        glfwCreateWindow(static_cast<int>(m_WindowSize.width),
                         static_cast<int>(m_WindowSize.height), "", nullptr, nullptr);

    glfwSetWindowUserPointer(m_GLFWwindow, this);
    glfwSetKeyCallback(m_GLFWwindow, vkWindow::onKeyCallback);
    glfwSetWindowSizeCallback(m_GLFWwindow, vkWindow::onWindowResized);
    glfwSetMouseButtonCallback(m_GLFWwindow, vkWindow::onMouseButtonCallback);
    glfwSetCursorPosCallback(m_GLFWwindow, vkWindow::onCursorPositionCallback);
    glfwSetScrollCallback(m_GLFWwindow, vkWindow::onMouseScrollCallback);

    glfwSetCharCallback(m_GLFWwindow, ImGui_ImplGlfw_CharCallback);

    m_camera.initDefaults(static_cast<float>(m_WindowSize.width)
                          / static_cast<float>(m_WindowSize.height));
}

void vkWindow::update(float deltaTime)
{
    glm::vec3 move(0.0f);
    if(keys.left)
        move.x += 1.0f;
    if(keys.right)
        move.x -= 1.0f;
    if(keys.forward)
        move.z += 1.0f;
    if(keys.back)
        move.z -= 1.0f;
    if(keys.up)
        move.y -= 1.0f;
    if(keys.down)
        move.y += 1.0f;

    m_camera.updateMovements(deltaTime, move);
    m_camera.update(deltaTime);
}

void vkWindow::setWindowTitle(const std::string& title)
{
    glfwSetWindowTitle(m_GLFWwindow, title.c_str());
}

void vkWindow::moveLightToCamera()
{
    m_vkctx->m_moveLight = true;
}

void vkWindow::handleKeyboardInput(int key, bool isPressed)
{
    if(key == GLFW_KEY_A)
    {
        keys.left = isPressed;
    }
    if(key == GLFW_KEY_S)
    {
        keys.back = isPressed;
    }
    if(key == GLFW_KEY_D)
    {
        keys.right = isPressed;
    }
    if(key == GLFW_KEY_W)
    {
        keys.forward = isPressed;
    }
    if(key == GLFW_KEY_Q)
    {
        keys.up = isPressed;
    }
    if(key == GLFW_KEY_E)
    {
        keys.down = isPressed;
    }
    if(key == GLFW_KEY_LEFT_SHIFT)
    {
        keys.l_shift = isPressed;
    }
    m_vkctx->m_cameraMoved = true;
}

void vkWindow::handleMouseButtonInput(int button, bool isPressed)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        mouseButtons.left = isPressed;
    }
    if(button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
        mouseButtons.middle = isPressed;
    }
    if(button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        mouseButtons.right = isPressed;
    }
}

void vkWindow::handleMouseCursorInput(double xpos, double ypos)
{
    mouse.lastPositon     = mouse.currentPosition;
    mouse.currentPosition = {static_cast<float>(xpos), static_cast<float>(ypos)};
    mouse.delta           = mouse.currentPosition - mouse.lastPositon;

    if(mouseButtons.left)
    {
        m_camera.updateMouseMovements(mouse.delta);
        if(mouse.delta != glm::vec2(0.0f))
        {
            m_vkctx->m_cameraMoved = true;
        }
    }
    mouse.delta = {0.0f, 0.0f};
}

void vkWindow::handleMouseScrollInput(double xpos, double ypos)
{
    m_camera.updateScroll(ypos);
}

void vkWindow::onErrorCallback(int error, const char* description)
{
    fprintf(stderr, "GLFW Error %d: %s\n");
}

void vkWindow::onWindowResized(GLFWwindow* window, int width, int height)
{
    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));

    // If dimensions are zero, window is propably minimized
    while(width == 0 || height == 0)
    {
        return;
    }

    app->m_WindowSize = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
    //app->m_vkRenderer->windowResizeCallback(app->m_WindowSize);
}

void vkWindow::onKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));

    app->m_vkctx->handleKeyPresses(key, action);
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if(key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        app->moveLightToCamera();
    }

    if(action == GLFW_PRESS)
    {
        app->handleKeyboardInput(key, true);
    }
    if(action == GLFW_RELEASE)
    {
        app->handleKeyboardInput(key, false);
    }
}

void vkWindow::onCursorPositionCallback(GLFWwindow* window, double xpos, double ypos)
{
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse || io.WantCaptureKeyboard)
    {
        return;
    }

    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));
    app->handleMouseCursorInput(xpos, ypos);
}

void vkWindow::onMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }


    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));

    if(action == GLFW_PRESS)
    {
        app->handleMouseButtonInput(button, true);
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            app->m_vkctx->m_cameraMoved = true;
            //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    }
    if(action == GLFW_RELEASE)
    {
        app->handleMouseButtonInput(button, false);
        if(button == GLFW_MOUSE_BUTTON_RIGHT)
        {
            //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
}

void vkWindow::onMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    ImGuiIO& io = ImGui::GetIO();
    if(io.WantCaptureMouse)
    {
        return;
    }

    auto app = reinterpret_cast<vkWindow*>(glfwGetWindowUserPointer(window));
    app->handleMouseScrollInput(xoffset, yoffset);
}
