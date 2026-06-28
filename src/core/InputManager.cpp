#include "core/InputManager.h"
#include "graphics/Camera.h"

InputManager::InputManager()
    : camera(nullptr),
      lastX(800.0f), lastY(600.0f),
      firstMouse(true)
{
}

void InputManager::setCamera(Camera* cam) {
    camera = cam;
}

void InputManager::processInput(GLFWwindow* window, float deltaTime) {
    if (!camera) return;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera->ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera->ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera->ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(RIGHT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        camera->ProcessKeyboard(UP, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        camera->ProcessKeyboard(DOWN, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void InputManager::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)window; (void)key; (void)scancode; (void)action; (void)mods;
}

void InputManager::mouseCallback(GLFWwindow* window, double xpos, double ypos) {
    if (!camera) return;
    if (firstMouse) {
        lastX = static_cast<float>(xpos);
        lastY = static_cast<float>(ypos);
        firstMouse = false;
    }
    float xoffset = static_cast<float>(xpos) - lastX;
    float yoffset = lastY - static_cast<float>(ypos);
    lastX = static_cast<float>(xpos);
    lastY = static_cast<float>(ypos);
    camera->ProcessMouseMovement(xoffset, yoffset);
}

void InputManager::scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (camera) {
        camera->ProcessMouseScroll(static_cast<float>(yoffset));
    }
}

void InputManager::framebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
