#ifndef QTECONDITIONS_H
#define QTECONDITIONS_H

#include <GLFW/glfw3.h>

class QTEConditions {
public:
    static bool isKeyPressed(GLFWwindow* window, int key);
    static bool isKeyHeld(GLFWwindow* window, int key);
};

#endif