#ifndef ANIMATION_CONDITIONS_H
#define ANIMATION_CONDITIONS_H

#include <GLFW/glfw3.h>

class AnimationConditions {
public:
    static bool isTKeyPressed(GLFWwindow* window) {
        return glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS;
    }

    static bool isTKeyReleased(GLFWwindow* window) {
        return glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE;
    }
};

#endif