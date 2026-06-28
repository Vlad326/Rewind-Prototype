#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <GLFW/glfw3.h>

class Camera;

class InputManager {
public:
    InputManager();

    void setCamera(Camera* camera);

    void processInput(GLFWwindow* window, float deltaTime);

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    void mouseCallback(GLFWwindow* window, double xpos, double ypos);
    void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    void framebufferSizeCallback(GLFWwindow* window, int width, int height);

private:
    Camera* camera;

    float lastX, lastY;
    bool firstMouse;
};

#endif