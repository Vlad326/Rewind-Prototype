#include "core/QTEConditions.h"
#include <unordered_map>

static std::unordered_map<int, bool> previousKeyState;

bool QTEConditions::isKeyPressed(GLFWwindow* window, int key) {
    bool current = glfwGetKey(window, key) == GLFW_PRESS;
    bool& prev = previousKeyState[key];
    bool pressed = current && !prev;
    prev = current;
    return pressed;
}

bool QTEConditions::isKeyHeld(GLFWwindow* window, int key) {
    return glfwGetKey(window, key) == GLFW_PRESS;
}
