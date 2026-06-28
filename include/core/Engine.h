#ifndef ENGINE_H
#define ENGINE_H

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <memory>
#include <cmath>
#include <cstdlib>

#include "graphics/Camera.h"
#include "core/Scene.h"
#include "graphics/Light.h"
#include "graphics/LightManager.h"
#include "resources/ModelLoader.h"
#include "resources/Model.h"
#include "resources/Mesh.h"
#include "graphics/Renderer.h"
#include "core/InputManager.h"
#include "graphics/PostProcessor.h"
#include "graphics/LightProbeManager.h"
#include "physics/PhysicsWorld.h"
#include "physics/CollisionComponent.h"
#include "animation/Animation.h"
#include "animation/AnimationManager.h"
#include "animation/AnimationBlender.h"
#include "animation/CameraShake.h"
#include "core/QTE.h"
#include "ui/FontManager.h"
#include "ui/UIFadeOverlay.h"
#include "ui/UIText.h"
#include "ui/UIPanel.h"
#include "audio/AudioManager.h"

class Engine {
public:
    Engine();
    ~Engine();

    bool init();
    int run();

private:
    enum class MemoryCinematicState {
        Idle,
        FadeOut,
        BlackScreen,
        Playing,
        FadeOutPost,
        BlackScreenPost,
        FadeInPost
    };

    enum class PaperMemoryState {
        Idle,
        Teleporting,
        Playing,
        Returning
    };

    enum class EndingState {
        Idle,
        KnockSound,
        WaitingForLocation,
        PlayingAnimation,
        ShowingChoices,
        WaitingForChoice
    };

    unsigned int screenWidth = 1200;
    unsigned int screenHeight = 800;

    Camera camera;
    float deltaTime;
    float lastFrame;
    float fps;
    int frameCount;
    float lastFpsUpdate;

    std::unique_ptr<Scene> exteriorScene;
    std::unique_ptr<Scene> interiorScene;
    Scene* currentScene = nullptr;

    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<InputManager> inputManager;
    std::unique_ptr<PostProcessor> postProcessor;
    std::unique_ptr<LightManager> lightManager;
    std::unique_ptr<PhysicsWorld> physicsWorld;
    std::unique_ptr<AnimationManager> animationManager;
    std::unique_ptr<FontManager> fontManager;
    std::unique_ptr<QTE> qte;
    std::unique_ptr<QTE> bowlQte;
    std::unique_ptr<QTE> paperQte;

    std::unique_ptr<UIFadeOverlay> fadeOverlay;

    std::unique_ptr<UIText> paperText;
    float paperTextAlpha = 0.0f;
    float paperTextFadeStartTime = 0.0f;
    bool paperTextFading = false;

    Model* characterModel = nullptr;

    Model* grass01Model = nullptr;
    Model* grass02Model = nullptr;

    Model* streetLamp01Model = nullptr;
    Model* streetLamp02Model = nullptr;

    Model* tree01Model = nullptr;
    Model* tree02Model = nullptr;

    Model* porchModel = nullptr;
    Model* houseExteriorModel = nullptr;

    Model* walkwayModel = nullptr;
    Model* fence01GroupModel = nullptr;
    
    Model* fog01Model = nullptr;
    Model* fog01GroupModel = nullptr;

    Model* borderO1Model = nullptr;

    Model* door01Model = nullptr;
    Model* doorFrame01Model = nullptr;

    Model* entranceHallModel = nullptr;

    Model* bowlModel = nullptr;
    Model* paperModel = nullptr;
    SceneObject* bowlObject = nullptr;
    SceneObject* paperObject = nullptr;

    Model* doorwayModel1 = nullptr;
    Model* doorwayModel2 = nullptr;
    SceneObject* doorwayObject1 = nullptr;
    SceneObject* doorwayObject2 = nullptr;

    Model* memoryCharacterModel = nullptr;
    
    Model* keyModel = nullptr;
    SceneObject* keyObject = nullptr;
    glm::vec3 keyPositionOffset = glm::vec3(0.0f, -0.5f, 0.0f);
    glm::vec3 keyRotationOffset = glm::vec3(0.0f, -15.0f-90.0f, 0.0f);

    Animation* cameraAnimation = nullptr;
    Animation* characterTransformAnimation = nullptr;
    Animation* prologueDoorAnimation = nullptr;
    Animation* prologueCharacterAnimation = nullptr;
    Animation* prologueHandleAnimation = nullptr;
    CameraShake cameraShake;

    Animation* memoryAnimation = nullptr;
    bool memoryActive = false;
    bool memoryTriggered = false;
    MemoryCinematicState memoryState = MemoryCinematicState::Idle;
    float memoryStartTime = 0.0f;
    float memorySavedExposure = 0.1f;
    float memorySavedZoom = 45.0f;

    const float memoryFadeOutDuration = 0.5f;
    const float memoryBlackScreenDuration = 0.3f;
    const float memoryFadeInDuration = 0.5f;
    const float memoryFadeOutPostDuration = 0.5f;
    const float memoryFadeInPostDuration = 0.5f;

    glm::vec3 memorySavedCameraPos;
    glm::vec3 memorySavedCameraFront;
    glm::vec3 memorySavedCameraUp;
    glm::vec3 memorySavedCameraRight;
    float memorySavedYaw;
    float memorySavedPitch;
    CameraShake memoryCameraShake;

    PaperMemoryState paperMemoryState = PaperMemoryState::Idle;
    float paperMemoryStartTime = 0.0f;
    bool paperMemoryTriggered = false;
    Animation* paperMemoryAnimation = nullptr;
    CameraShake paperCameraShake;
    glm::vec3 paperSavedCameraPos;
    glm::vec3 paperSavedCameraFront;
    glm::vec3 paperSavedCameraUp;
    glm::vec3 paperSavedCameraRight;
    float paperSavedYaw;
    float paperSavedPitch;
    float paperSavedZoom;

    int hope = 0;

    bool endingTriggered = false;
    bool endingKnockSoundPlayed = false;
    float endingStateStartTime = 0.0f;
    EndingState endingState = EndingState::Idle;
    Animation* endingCameraAnimation = nullptr;
    CameraShake endingCameraShake;
    bool endingCameraAnimationStarted = false;

    Animation* idleAnimation = nullptr;
    Animation* walkAnimation = nullptr;
    std::unique_ptr<AnimationBlender> characterBlender;

    SceneObject* prologueDoor01Obj_001 = nullptr;
    Mesh* doorHandleMesh = nullptr;
    
    int streetLampShadowResolution = 256;


    bool fullscreen = false;
    int windowedPosX = 0, windowedPosY = 0;
    int windowedWidth = 1200, windowedHeight = 800;

    bool cutsceneActive = false;
    SceneObject* cutsceneCharacter = nullptr;

    std::unique_ptr<SceneObject> characterObjectPtr;
    SceneObject* characterObject = nullptr;

    bool freeCamera = false;
    bool freeCameraInitialized = false;

    bool thirdPersonCameraActive = false;
    bool thirdPersonTransitioning = false;
    float thirdPersonRightOffset = 0.1f;
    float thirdPersonDistance = 1.1f;
    const float eyeHeight = 1.65f;
    glm::vec3 thirdPersonTargetPos;
    glm::vec3 thirdPersonTargetFront;
    glm::vec3 thirdPersonTransitionStartPos;
    glm::vec3 thirdPersonTransitionStartFront;
    float thirdPersonTransitionStartTime = 0.0f;
    float interiorEntryTime = 0.0f;
    bool thirdPerson2SecPassed = false;
    bool characterJustTeleported = false;
    glm::vec3 smoothedCameraPos;

    glm::vec3 cleanCameraPosition;
    glm::vec3 cleanCameraFront;
    glm::vec3 cleanCameraUp;
    glm::vec3 cleanCameraRight;

    bool prologueDoorAnimationStarted = false;
    bool prologueCharacterAnimationStarted = false;

    bool qteActive = false;
    float qteProgress = 0.0f;
    bool qteCompleted = false;
    bool keyHiddenAndTeleported = false;
    bool transitionToInteriorDone = false;

    bool characterControlEnabled = false;
    float currentCharacterSpeed = 0.0f;
    const float maxCharacterSpeed = 1.5f;

    Model* dogModel = nullptr;
    SceneObject* dogObject = nullptr;
    SceneObject* memoryCharacterObject = nullptr;
    Animation* dogAnimation = nullptr;
    std::unique_ptr<AnimationManager> dogAnimManager;
    Animation* memoryCharacterIdleAnimation = nullptr;
    std::unique_ptr<AnimationManager> memoryCharacterAnimManager;
    bool memoryDogPointLightActive = false;

    std::unique_ptr<AudioManager> audioManager;
    ALuint ambientBuffer = 0;
    ALuint ambientSource = 0;
    ALuint dogPantingBuffer = 0;
    ALuint dogPantingSource = 0;
    
    std::vector<ALuint> footstepBuffers;
    ALuint footstepSourceLeft = 0;
    ALuint footstepSourceRight = 0;
    float lastCycleTime = 0.0f;
    int footstepPhase = -1;       
    bool ambientPlaying = false;

    void startCutscene(SceneObject* character);
    void endCutscene();
    
    void updateKeyAttachment();

    void switchToScene(Scene* newScene);

    void handleFootsteps(float deltaTime);

    void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void keyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseCallbackWrapper(GLFWwindow* window, double xpos, double ypos);
    static void scrollCallbackWrapper(GLFWwindow* window, double xoffset, double yoffset);
    static void framebufferSizeCallbackWrapper(GLFWwindow* window, int w, int h);

    void onWindowResize(int width, int height);
    void updateFPS(GLFWwindow* window);
};

#endif