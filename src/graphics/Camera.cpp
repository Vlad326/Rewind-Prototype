// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 Kolmogorov Vlad
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "graphics/Camera.h"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch) 
    : front(glm::vec3(0.0f, 0.0f, -1.0f)), movementSpeed(2.5f), 
      mouseSensitivity(0.1f), zoom(45.0f), exposure(1.0f)
{
    this->position = position;
    this->worldUp = up;
    this->yaw = yaw;
    this->pitch = pitch;
    updateCameraVectors();
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::lookAt(position, position + front, up);
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
    float velocity = movementSpeed * deltaTime;
    
    if (direction == FORWARD)
        position += front * velocity;
    if (direction == BACKWARD)
        position -= front * velocity;
    if (direction == LEFT)
        position -= right * velocity;
    if (direction == RIGHT)
        position += right * velocity;
    if (direction == UP)
        position += worldUp * velocity;
    if (direction == DOWN)
        position -= worldUp * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (constrainPitch) {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float yoffset) {
    zoom -= (float)yoffset;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);
    
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::extractFrustumPlanes(const glm::mat4& projection, FrustumPlane planes[6]) const {
    glm::mat4 viewProj = projection * GetViewMatrix();

    planes[0].normal.x = viewProj[0][3] + viewProj[0][0];
    planes[0].normal.y = viewProj[1][3] + viewProj[1][0];
    planes[0].normal.z = viewProj[2][3] + viewProj[2][0];
    planes[0].distance = viewProj[3][3] + viewProj[3][0];

    planes[1].normal.x = viewProj[0][3] - viewProj[0][0];
    planes[1].normal.y = viewProj[1][3] - viewProj[1][0];
    planes[1].normal.z = viewProj[2][3] - viewProj[2][0];
    planes[1].distance = viewProj[3][3] - viewProj[3][0];

    planes[2].normal.x = viewProj[0][3] + viewProj[0][1];
    planes[2].normal.y = viewProj[1][3] + viewProj[1][1];
    planes[2].normal.z = viewProj[2][3] + viewProj[2][1];
    planes[2].distance = viewProj[3][3] + viewProj[3][1];

    planes[3].normal.x = viewProj[0][3] - viewProj[0][1];
    planes[3].normal.y = viewProj[1][3] - viewProj[1][1];
    planes[3].normal.z = viewProj[2][3] - viewProj[2][1];
    planes[3].distance = viewProj[3][3] - viewProj[3][1];

    planes[4].normal.x = viewProj[0][3] + viewProj[0][2];
    planes[4].normal.y = viewProj[1][3] + viewProj[1][2];
    planes[4].normal.z = viewProj[2][3] + viewProj[2][2];
    planes[4].distance = viewProj[3][3] + viewProj[3][2];

    planes[5].normal.x = viewProj[0][3] - viewProj[0][2];
    planes[5].normal.y = viewProj[1][3] - viewProj[1][2];
    planes[5].normal.z = viewProj[2][3] - viewProj[2][2];
    planes[5].distance = viewProj[3][3] - viewProj[3][2];

    for (int i = 0; i < 6; ++i) {
        float len = glm::length(planes[i].normal);
        planes[i].normal /= len;
        planes[i].distance /= len;
    }
}