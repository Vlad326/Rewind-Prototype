cmake_minimum_required(VERSION 3.10)
project(MyEngineCPP)

set(CMAKE_CXX_STANDARD 17)

set(MINGW_PREFIX "/ucrt64")
set(CMAKE_PREFIX_PATH "${MINGW_PREFIX}")

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW REQUIRED glfw3)
pkg_check_modules(GLEW REQUIRED glew)
pkg_check_modules(ASSIMP REQUIRED assimp)
pkg_check_modules(OPENAL REQUIRED openal)
pkg_check_modules(FREETYPE REQUIRED freetype2)
pkg_check_modules(BULLET REQUIRED bullet)

include_directories(
    include
    third_party/dr_wav    # <-- Добавлено
    third_party/stb       # <-- Добавлено
    third_party/json
    ${ASSIMP_INCLUDE_DIRS}
    ${GLFW_INCLUDE_DIRS}
    ${GLEW_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIRS}
    ${FREETYPE_INCLUDE_DIRS}
    ${BULLET_INCLUDE_DIRS}
)

add_executable(main
    src/core/main.cpp
    src/core/Engine.cpp
    src/core/InputManager.cpp
    src/core/Scene.cpp
    src/core/QTE.cpp
    src/core/QTEConditions.cpp

    src/graphics/Renderer.cpp
    src/graphics/Camera.cpp
    src/graphics/Light.cpp
    src/graphics/ShaderManager.cpp
    src/graphics/LightProbeManager.cpp
    src/graphics/PostProcessor.cpp
    src/graphics/LightManager.cpp

    src/resources/ModelLoader.cpp
    src/resources/Model.cpp
    src/resources/Mesh.cpp
    src/resources/GeometryGenerator.cpp
    src/resources/TextureGenerator.cpp
    
    src/animation/AnimationManager.cpp
    src/animation/AnimationBlender.cpp
    src/animation/AnimationStateMachine.cpp
    src/animation/Animation.cpp
    src/animation/BoneVisualizer.cpp
    src/animation/CameraShake.cpp

    src/physics/CollisionComponent.cpp
    src/physics/PhysicsWorld.cpp

    src/audio/AudioManager.cpp

    src/ui/UIText.cpp
    src/ui/UIButton.cpp
    src/ui/UITexture.cpp
    src/ui/UIPanel.cpp
    src/ui/FontManager.cpp
    src/ui/UIWidget.cpp
    src/ui/UIFadeOverlay.cpp
)


target_link_libraries(main
    ${ASSIMP_LIBRARIES}
    ${GLFW_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${OPENAL_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${BULLET_LIBRARIES}
    z
)

if(WIN32)
    target_link_libraries(main opengl32 gdi32 user32)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -march=native -ffast-math -Wall -Wextra")