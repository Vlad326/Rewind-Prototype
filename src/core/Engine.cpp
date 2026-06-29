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

#include "core/Engine.h"
#include <iomanip>
#include <cstdlib>
#include <ctime>

Engine::Engine()
    : camera(glm::vec3(0.0f, 3.0f, 10.0f)),
      deltaTime(0.0f),
      lastFrame(0.0f),
      fps(0.0f),
      frameCount(0),
      lastFpsUpdate(0.0f),
      animationManager(nullptr),
      inputManager(std::make_unique<InputManager>()),
      characterModel(nullptr),
      cameraAnimation(nullptr),
      characterTransformAnimation(nullptr),
      prologueDoorAnimation(nullptr),
      prologueCharacterAnimation(nullptr),
      prologueHandleAnimation(nullptr),
      prologueDoor01Obj_001(nullptr),
      doorHandleMesh(nullptr),
      prologueDoorAnimationStarted(false),
      prologueCharacterAnimationStarted(false),
      freeCamera(false),
      freeCameraInitialized(false),
      keyModel(nullptr),
      keyObject(nullptr),
      qteActive(false),
      qteProgress(0.0f),
      qteCompleted(false),
      keyHiddenAndTeleported(false),
      transitionToInteriorDone(false),
      idleAnimation(nullptr),
      walkAnimation(nullptr),
      characterControlEnabled(false),
      currentCharacterSpeed(0.0f),
      bowlModel(nullptr),
      paperModel(nullptr),
      bowlObject(nullptr),
      paperObject(nullptr),
      
      memoryAnimation(nullptr),
      memoryActive(false),
      memoryTriggered(false),
      memoryState(MemoryCinematicState::Idle),
      hope(0),
      dogModel(nullptr),
      dogObject(nullptr),
      memoryCharacterObject(nullptr),
      dogAnimation(nullptr),
      dogAnimManager(nullptr),
      memoryCharacterIdleAnimation(nullptr),
      memoryCharacterAnimManager(nullptr),
      memoryDogPointLightActive(false),
      paperMemoryState(PaperMemoryState::Idle),
      paperMemoryStartTime(0.0f),
      paperMemoryTriggered(false),
      paperMemoryAnimation(nullptr),
      paperTextAlpha(0.0f),
      paperTextFadeStartTime(0.0f),
      paperTextFading(false)
{
    cleanCameraPosition = camera.position;
    cleanCameraFront    = camera.front;
    cleanCameraUp       = camera.up;
    cleanCameraRight    = camera.right;

    srand(static_cast<unsigned int>(time(nullptr)));
}

Engine::~Engine() {
    if (audioManager) {
        if (ambientSource) {
            audioManager->stopSource(ambientSource);
            audioManager->deleteSource(ambientSource);
        }
        if (dogPantingSource) {
            audioManager->stopSource(dogPantingSource);
            audioManager->deleteSource(dogPantingSource);
        }
        if (footstepSourceLeft) {
            audioManager->stopSource(footstepSourceLeft);
            audioManager->deleteSource(footstepSourceLeft);
        }
        if (footstepSourceRight) {
            audioManager->stopSource(footstepSourceRight);
            audioManager->deleteSource(footstepSourceRight);
        }
    }

    delete cameraAnimation;
    delete characterTransformAnimation;
    delete prologueDoorAnimation;
    delete prologueCharacterAnimation;
    delete prologueHandleAnimation;
    delete idleAnimation;
    delete walkAnimation;
    delete memoryAnimation;
    delete dogAnimation;
    delete memoryCharacterIdleAnimation;
    delete memoryCharacterModel;
    delete paperMemoryAnimation;
    glfwTerminate();
    std::cout << "Engine resources cleaned up successfully" << std::endl;
}

void Engine::startCutscene(SceneObject* character) {
    cutsceneActive = true;
    cutsceneCharacter = character;
    std::cout << "[Cutscene] Started" << std::endl;
}

void Engine::endCutscene() {
    cutsceneActive = false;
    cutsceneCharacter = nullptr;
    std::cout << "[Cutscene] Ended" << std::endl;
}

void Engine::switchToScene(Scene* newScene) {
    if (currentScene == newScene) return;
    std::cout << "[Engine] Switching scene..." << std::endl;

    if (physicsWorld) {
        physicsWorld->clearStaticObjects();
        int staticCount = 0;
        int skippedCount = 0;
        for (SceneObject* obj : newScene->getObjects()) {
            if (obj->name.find("ProbeDebug") != std::string::npos) {
                skippedCount++;
                continue;
            }
            if (obj->isStatic() && obj->isVisible()) {
                physicsWorld->addStaticMeshCollider(obj);
                std::cout << "  Added static collider: " << obj->name << std::endl;
                staticCount++;
            }
        }
        std::cout << "[Physics] Added " << staticCount << " static colliders, skipped " 
                  << skippedCount << " probe debug objects" << std::endl;
    }

    if (lightManager) {
        lightManager->setScene(*newScene);
    }

    auto* probeMgr = newScene->getLightProbeManager();
    if (probeMgr && renderer) {
        GLuint ssbo = probeMgr->getProbeSSBO();
        if (ssbo) {
            renderer->setLightProbeSSBO(ssbo,
                                        probeMgr->getProbeGridResolution(),
                                        probeMgr->getProbeGridMin(),
                                        probeMgr->getProbeGridSize());
        }
    }

    currentScene = newScene;
    std::cout << "[Engine] Scene switched." << std::endl;
}

void Engine::updateKeyAttachment() {
    if (!keyObject || !characterModel) return;

    const std::string boneName = "mixamorig:RightHandThumb4";
    const std::vector<Bone>& skeleton = characterModel->getSkeleton();
    
    Animation* currentAnim = nullptr;
    if (characterBlender && characterBlender->getSourceCount() > 0) {
        currentAnim = walkAnimation;
    } else if (animationManager) {
        currentAnim = animationManager->getCurrentAnimation();
    }
    
    if (!currentAnim) return;

    int boneIndex = -1;
    for (size_t i = 0; i < skeleton.size(); ++i) {
        if (skeleton[i].name == boneName) {
            boneIndex = (int)i;
            break;
        }
    }
    if (boneIndex == -1) {
        static bool warned = false;
        if (!warned) {
            std::cerr << "[Key] Bone '" << boneName << "' not found in skeleton!" << std::endl;
            warned = true;
        }
        return;
    }

    float animTime = currentAnim->getCurrentTime();

    std::vector<glm::mat4> localTransforms(skeleton.size(), glm::mat4(1.0f));
    
    if (characterBlender && characterBlender->getSourceCount() > 0) {
        for (size_t i = 0; i < skeleton.size(); ++i) {
            localTransforms[i] = characterBlender->getBlendedLocalTransform(skeleton[i].name);
        }
    } else {
        for (size_t i = 0; i < skeleton.size(); ++i) {
            localTransforms[i] = currentAnim->getBoneTransform(skeleton[i].name, animTime);
        }
    }

    std::vector<glm::mat4> globalTransforms(skeleton.size(), glm::mat4(1.0f));
    for (size_t i = 0; i < skeleton.size(); ++i) {
        int parent = skeleton[i].parentId;
        if (parent >= 0 && parent < (int)skeleton.size())
            globalTransforms[i] = globalTransforms[parent] * localTransforms[i];
        else
            globalTransforms[i] = localTransforms[i];
    }

    glm::mat4 characterWorld = glm::mat4(1.0f);
    characterWorld = glm::translate(characterWorld, characterObject->position);
    characterWorld = glm::rotate(characterWorld, glm::radians(characterObject->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    characterWorld = glm::rotate(characterWorld, glm::radians(characterObject->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    characterWorld = glm::rotate(characterWorld, glm::radians(characterObject->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    characterWorld = glm::scale(characterWorld, characterObject->scale);

    glm::vec3 boneModelPos = glm::vec3(globalTransforms[boneIndex][3]);
    glm::mat3 boneModelRot = glm::mat3(globalTransforms[boneIndex]);
    glm::vec3 boneWorldPos = glm::vec3(characterWorld * glm::vec4(boneModelPos, 1.0f));
    
    glm::mat3 charWorldRot = glm::mat3(characterWorld);
    glm::mat3 boneWorldRot = charWorldRot * boneModelRot;
    
    glm::vec3 worldOffset = boneWorldRot * keyPositionOffset;
    glm::vec3 finalWorldPos = boneWorldPos + worldOffset;
    
    glm::quat rotationOffset = glm::quat(glm::radians(keyRotationOffset));
    glm::quat boneWorldQuat = glm::quat_cast(boneWorldRot);
    glm::quat finalRotation = boneWorldQuat * rotationOffset;

    keyObject->position = finalWorldPos;
    keyObject->rotation = glm::degrees(glm::eulerAngles(finalRotation));
    
    static int debugCounter = 0;
    if (debugCounter++ % 60 == 0) {
        std::cout << "[Key] Bone '" << boneName << "' world pos: " 
                  << boneWorldPos.x << ", " << boneWorldPos.y << ", " << boneWorldPos.z
                  << " | Final pos with offset: "
                  << finalWorldPos.x << ", " << finalWorldPos.y << ", " << finalWorldPos.z
                  << " | Rotation: "
                  << keyObject->rotation.x << ", " << keyObject->rotation.y << ", " << keyObject->rotation.z << std::endl;
    }
}

bool Engine::init() {
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW" << std::endl; return false; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Character Viewer", monitor, NULL);
    if (!window) { std::cerr << "Failed to create GLFW window" << std::endl; glfwTerminate(); return false; }
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, this);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) { std::cerr << "Failed to initialize GLEW" << std::endl; return false; }

    glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
    glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
    screenWidth  = mode->width;
    screenHeight = mode->height;

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "=== Controls ===" << std::endl;
    std::cout << "Camera: WASD (move), Q/E (up/down), Mouse (look)" << std::endl;
    std::cout << "Fullscreen: F11" << std::endl;
    std::cout << "Toggle Free Camera: F" << std::endl;
    std::cout << "Toggle Hitbox Debug: H" << std::endl;
    std::cout << "Toggle Light Probes: P" << std::endl;
    std::cout << "Exit: ESC or Close button" << std::endl;

    camera.exposure = 0.08f;
    inputManager->setCamera(&camera);
    glfwSetFramebufferSizeCallback(window, framebufferSizeCallbackWrapper);
    glfwSetCursorPosCallback(window, mouseCallbackWrapper);
    glfwSetScrollCallback(window, scrollCallbackWrapper);
    glfwSetKeyCallback(window, keyCallbackWrapper);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    renderer = std::make_unique<Renderer>();
    std::string hdrPath = "../content/hdri/NightSky4K.hdr";
    if (!renderer->init(hdrPath)) { std::cerr << "Failed to initialize renderer" << std::endl; return false; }

    renderer->setupPostProcessing(screenWidth, screenHeight);
    postProcessor = std::make_unique<PostProcessor>(*renderer);

    renderer->setSkyboxIntensity(0.05f);
    renderer->setIBLIntensity(0.3f);

    fontManager = std::make_unique<FontManager>();

    exteriorScene = std::make_unique<Scene>();
    interiorScene = std::make_unique<Scene>();
    currentScene = exteriorScene.get();
    std::cout << "[Engine] Created exteriorScene and interiorScene" << std::endl;

    return true;
}

int Engine::run() {
    GLFWwindow* window = glfwGetCurrentContext();
    if (!window) return -1;

    std::cout << "\nLoading character model..." << std::endl;


    characterModel = ModelLoader::loadFBX(
        "../content/character/main/anim_walking.fbx",
        "../content/character/main/textures"
    );

    if (characterModel && characterModel->getMeshCount() > 0) {
        for (int i = 0; i < characterModel->getMeshCount(); i++) {
            Mesh* mesh = characterModel->getMesh(i);
            if (mesh) {
                mesh->material.hasRoughnessTexture = false;
                mesh->material.roughness = 0.7f;
                mesh->material.hasSpecularTexture = false;
                mesh->material.specularFactor = glm::vec3(0.04f);
                mesh->material.alphaThreshold = 0.5f;
                mesh->isStatic = false;
            }
        }
    }

    grass02Model = ModelLoader::loadFBX(
        "../content/models/Grass_02/models/Grass_02.fbx"
    );

    grass01Model = ModelLoader::loadFBX(
        "../content/models/Grass_01/models/Grass_01.fbx"
    );
    
    streetLamp01Model = ModelLoader::loadFBX(
        "../content/models/StreetLamp_01/models/StreetLamp_01.fbx"
    );

    streetLamp02Model = ModelLoader::loadFBX(
        "../content/models/StreetLamp_02/models/StreetLamp_02.fbx"
    );

    tree01Model = ModelLoader::loadFBX(
        "../content/models/Tree_01/models/Tree_01.fbx"
    );

    tree02Model = ModelLoader::loadFBX(
        "../content/models/Tree_02/models/Tree_02.fbx"
    );

    porchModel = ModelLoader::loadFBX(
        "../content/models/Porch/models/Porch.fbx"
    );

    houseExteriorModel = ModelLoader::loadFBX(
        "../content/models/HouseExterior/models/HouseExterior.fbx"
    );
    
    walkwayModel = ModelLoader::loadFBX(
        "../content/models/Walkway/models/Walkway.fbx"
    );

    fence01GroupModel = ModelLoader::loadFBX(
        "../content/models/Fence_01_Group/models/Fence_01_Group.fbx"
    );

    fog01Model = ModelLoader::loadFBX(
        "../content/models/Fog_01/models/Fog_01.fbx"
    );

    fog01GroupModel = ModelLoader::loadFBX(
        "../content/models/Fog_01_Group/models/Fog_01_Group.fbx"
    );

    borderO1Model = ModelLoader::loadFBX(
        "../content/models/Border_01/models/Border_01.fbx"
    );

    door01Model = ModelLoader::loadFBX(
        "../content/models/Door_01/models/Door_01.fbx"
    );

    doorFrame01Model = ModelLoader::loadFBX(
        "../content/models/DoorFrame_01/models/DoorFrame_01.fbx"
    );

    doorwayModel1 = ModelLoader::loadFBX(
        "../content/models/Doorway/models/Doorway.fbx"
    );
    doorwayModel2 = ModelLoader::loadFBX(
        "../content/models/Doorway/models/Doorway.fbx"
    );


    std::cout << "\nLoading interior model..." << std::endl;
    entranceHallModel = ModelLoader::loadFBX(
        "../content/models/HouseInterior/EntranceHall/models/EntranceHall.fbx",
        "../content/models/HouseInterior/EntranceHall/textures"
    );
    
    if (entranceHallModel) {
        std::cout << "EntranceHall model loaded. Meshes: " << entranceHallModel->getMeshCount() << std::endl;
        
        for (int i = 0; i < entranceHallModel->getMeshCount(); i++) {
            Mesh* mesh = entranceHallModel->getMesh(i);
            if (mesh) {
                mesh->isStatic = true;
            }
        }
        
        SceneObject* entranceHallObj = interiorScene->addObject(entranceHallModel, "EntranceHall");
        entranceHallObj->position = glm::vec3(0.0f, 0.0f, 0.0f);
        entranceHallObj->scale = glm::vec3(1.0f, 1.0f, 1.0f);
        entranceHallObj->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        entranceHallObj->setCastShadows(true);
    } else {
        std::cerr << "[Engine] Failed to load EntranceHall model!" << std::endl;
    }

    std::cout << "\nLoading kitchen model..." << std::endl;
    Model* kitchenModel = ModelLoader::loadFBX(
        "../content/models/HouseInterior/Kitchen/models/Kitchen.fbx",
        "../content/models/HouseInterior/Kitchen/textures"
    );
    
    if (kitchenModel) {
        std::cout << "Kitchen model loaded. Meshes: " << kitchenModel->getMeshCount() << std::endl;
        
        for (int i = 0; i < kitchenModel->getMeshCount(); i++) {
            Mesh* mesh = kitchenModel->getMesh(i);
            if (mesh) {
                mesh->isStatic = true;
            }
        }
        
        SceneObject* kitchenObj = interiorScene->addObject(kitchenModel, "Kitchen");
        kitchenObj->position = glm::vec3(0.0f, 0.0f, 0.0f);
        kitchenObj->scale = glm::vec3(1.0f, 1.0f, 1.0f);
        kitchenObj->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        kitchenObj->setCastShadows(true);
    } else {
        std::cerr << "[Engine] Failed to load Kitchen model!" << std::endl;
    }

    std::cout << "\nLoading Bowl model..." << std::endl;
    bowlModel = ModelLoader::loadFBX(
        "../content/models/Bowl/models/Bowl.fbx"
    );
    if (bowlModel) {
        std::cout << "Bowl model loaded. Meshes: " << bowlModel->getMeshCount() << std::endl;
        
        for (int i = 0; i < bowlModel->getMeshCount(); i++) {
            Mesh* mesh = bowlModel->getMesh(i);
            if (mesh) {
                mesh->isStatic = true;
                mesh->material.hasRoughnessTexture = false;
                mesh->material.albedoFactor = glm::vec3(0.1f, 0.1f, 0.1f);
                mesh->material.roughness = 0.2f;
                mesh->material.metallic = 1.0f;
            }
        }
        
        bowlObject = interiorScene->addObject(bowlModel, "Bowl");
        bowlObject->position = glm::vec3(-5.16111f, 0.880862f, 4.61704f);
        bowlObject->scale    = glm::vec3(1.0f);
        bowlObject->rotation = glm::vec3(0.0f);
        bowlObject->setCastShadows(true);
        std::cout << "[Engine] Bowl (metal dog bowl) added to interiorScene." << std::endl;
    } else {
        std::cerr << "[Engine] Failed to load Bowl model!" << std::endl;
    }

    std::cout << "\nLoading Paper model..." << std::endl;
    paperModel = ModelLoader::loadFBX(
        "../content/models/Paper/models/Paper.fbx"
    );
    if (paperModel) {
        std::cout << "Paper model loaded. Meshes: " << paperModel->getMeshCount() << std::endl;
        for (int i = 0; i < paperModel->getMeshCount(); i++) {
            Mesh* mesh = paperModel->getMesh(i);
            if (mesh) mesh->isStatic = true;
        }
        paperObject = interiorScene->addObject(paperModel, "Paper");
        paperObject->position = glm::vec3(0.904088f, 0.789154f, 5.40526f);
        paperObject->scale    = glm::vec3(1.0f);
        paperObject->rotation = glm::vec3(0.0f, -102.997f, 0.0f);
        paperObject->setCastShadows(true);
        std::cout << "[Engine] Paper added to interiorScene." << std::endl;
    } else {
        std::cerr << "[Engine] Failed to load Paper model!" << std::endl;
    }

    std::cout << "\nLoading Dog model..." << std::endl;
    dogModel = ModelLoader::loadFBX(
        "../content/character/Dog/models/Dog.fbx",
        "../content/character/Dog/textures"
    );
    if (dogModel) {
        std::cout << "Dog model loaded. Meshes: " << dogModel->getMeshCount() << std::endl;
        std::vector<Bone> dogSkeleton = ModelLoader::loadSkeleton(
            "../content/character/Dog/models/Dog.fbx"
        );
        if (!dogSkeleton.empty()) {
            std::cout << "Dog skeleton loaded with " << dogSkeleton.size() << " bones" << std::endl;
            dogModel->setSkeleton(dogSkeleton);
        }
        dogAnimation = ModelLoader::loadAnimation(
            "../content/character/Dog/models/Dog.fbx"
        );
        if (dogAnimation) {
            dogAnimation->setLooping(true);
            dogAnimation->play();
            std::cout << "Dog animation loaded (looping)." << std::endl;
        }
        dogObject = interiorScene->addObject(dogModel, "Dog");
        dogObject->position = glm::vec3(-4.44153f, 0.0f, 5.40058f);
        dogObject->rotation = glm::vec3(90.0f, 180.0f, 0.0f);
        dogObject->scale = glm::vec3(1.0f, 1.0f, 1.0f);
        dogObject->setVisible(false);
        dogObject->setCastShadows(true);
        
        if (dogAnimation) {
            dogAnimManager = std::make_unique<AnimationManager>();
            dogAnimManager->setAnimation(dogAnimation, dogModel);
        }
    } else {
        std::cerr << "[Engine] Failed to load Dog model!" << std::endl;
    }

    memoryCharacterModel = ModelLoader::loadFBX(
        "../content/character/main/anim_walking.fbx",
        "../content/character/main/textures"
    );

    if (memoryCharacterModel) {
        if (memoryCharacterModel->getMeshCount() > 0) {
            for (int i = 0; i < memoryCharacterModel->getMeshCount(); i++) {
                Mesh* mesh = memoryCharacterModel->getMesh(i);
                if (mesh) {
                    mesh->material.hasRoughnessTexture = false;
                    mesh->material.roughness = 0.7f;
                    mesh->material.hasSpecularTexture = false;
                    mesh->material.specularFactor = glm::vec3(0.04f);
                    mesh->material.alphaThreshold = 0.5f;
                    mesh->isStatic = false;
                }
            }
        }
        
        std::vector<Bone> memorySkeleton = ModelLoader::loadSkeleton(
            "../content/character/main/anim_walking.fbx"
        );
        if (!memorySkeleton.empty()) {
            std::cout << "Memory character skeleton loaded with " << memorySkeleton.size() << " bones" << std::endl;
            memoryCharacterModel->setSkeleton(memorySkeleton);
            memoryCharacterModel->setShowBones(true);
        }
        
        memoryCharacterObject = interiorScene->addObject(memoryCharacterModel, "MemoryCharacter");
        memoryCharacterObject->position = glm::vec3(-4.21081f, 0.0f, 3.58422f);
        memoryCharacterObject->scale = characterObject ? characterObject->scale : glm::vec3(0.004818f);
        memoryCharacterObject->setVisible(false);
        memoryCharacterObject->setCastShadows(true);

        doorwayObject1 = interiorScene->addObject(doorwayModel1, "Doorway_001");
        doorwayObject1->position = glm::vec3(0.0f, 0.0f, 0.0f);
        doorwayObject1->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        doorwayObject1->scale    = glm::vec3(1.0f, 1.0f, 1.0f);

        doorwayObject2 = interiorScene->addObject(doorwayModel2, "Doorway_002");
        doorwayObject2->position = glm::vec3(0.0f, 0.0f, 7.7f);
        doorwayObject2->rotation = glm::vec3(0.0f, 0.0f, 0.0f);
        doorwayObject2->scale    = glm::vec3(1.0f, 1.0f, 1.0f);
        
        memoryCharacterIdleAnimation = ModelLoader::loadAnimation("../content/character/main/anim_idle.fbx");
        if (memoryCharacterIdleAnimation) {
            memoryCharacterIdleAnimation->setLooping(true);
            memoryCharacterIdleAnimation->play();
            std::cout << "Memory character idle animation loaded." << std::endl;
        }
        
        memoryCharacterAnimManager = std::make_unique<AnimationManager>();
        if (memoryCharacterIdleAnimation) {
            memoryCharacterAnimManager->setAnimation(memoryCharacterIdleAnimation, memoryCharacterModel);
        }
        
        std::cout << "[Engine] Memory character object created with SEPARATE model and hidden. Will use idle animation." << std::endl;
    } else {
        std::cerr << "[Engine] Failed to load memory character model!" << std::endl;
    }


    bowlQte = std::make_unique<QTE>('E', *fontManager, "../content/ui/fonts/Arial.ttf", 48);
    bowlQte->setMode(QTE_MODE_PRESS);
    bowlQte->setFillColor(glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
    bowlQte->setOutlineColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    bowlQte->setOutlineThickness(3);
    bowlQte->setCornerLength(0.15f);
    bowlQte->setFontSizeFraction(0.5f);
    bowlQte->setTextColor(glm::vec3(1.0f, 1.0f, 1.0f));
    bowlQte->setOneShot(true);
    bowlQte->setActive(false);
    std::cout << "[Engine] Bowl QTE created (PRESS mode, one-shot)." << std::endl;

    paperQte = std::make_unique<QTE>('E', *fontManager, "../content/ui/fonts/Arial.ttf", 48);
    paperQte->setMode(QTE_MODE_PRESS);
    paperQte->setFillColor(glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
    paperQte->setOutlineColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
    paperQte->setOutlineThickness(3);
    paperQte->setCornerLength(0.15f);
    paperQte->setFontSizeFraction(0.5f);
    paperQte->setTextColor(glm::vec3(1.0f, 1.0f, 1.0f));
    paperQte->setOneShot(true);
    paperQte->setActive(false);
    std::cout << "[Engine] Paper QTE created (PRESS mode, one-shot)." << std::endl;

    paperText = std::make_unique<UIText>(*fontManager, "../content/ui/fonts/Arial.ttf", 48);
    paperText->setText("Buy food for River");
    paperText->setColor(glm::vec3(1.0f, 1.0f, 1.0f));
    paperText->setScreenSize(screenWidth, screenHeight);
    paperText->setAlpha(0.0f);
    std::cout << "[Engine] Paper UI Text created." << std::endl;


    Light* interiorRectLight01 = interiorScene->addRectLight("Interior Rect Light 01");
    interiorRectLight01->position = glm::vec3(-0.037077f, 2.49269f, 5.7212f);
    interiorRectLight01->intensity = 15.0f;
    interiorRectLight01->color = glm::vec3(1.0f, 0.95f, 0.9f);
    interiorRectLight01->temperature = 5000.0f;
    interiorRectLight01->setRectLight(1.0f, 1.0f);
    interiorRectLight01->setCastShadows(true);
    interiorRectLight01->shadowIntensity = 0.6f;
    interiorRectLight01->shadowSoftness = 2.0f;
    {
        glm::vec3 localDir(0.0f, -1.0f, 0.0f);
        glm::vec3 localUp(0.0f, 0.0f, 1.0f);
        interiorRectLight01->direction = glm::normalize(localDir);
        interiorRectLight01->up = glm::normalize(localUp);
        interiorRectLight01->right = glm::normalize(glm::cross(interiorRectLight01->direction, interiorRectLight01->up));
        interiorRectLight01->up = glm::normalize(glm::cross(interiorRectLight01->right, interiorRectLight01->direction));
    }

    Light* interiorRectLight02 = interiorScene->addRectLight("Interior Rect Light 02");
    interiorRectLight02->position = glm::vec3(-0.037077f, 2.49269f, 2.96778f);
    interiorRectLight02->intensity = 15.0f;
    interiorRectLight02->color = glm::vec3(1.0f, 0.95f, 0.9f);
    interiorRectLight02->temperature = 5000.0f;
    interiorRectLight02->setRectLight(1.0f, 1.0f);
    interiorRectLight02->setCastShadows(true);
    interiorRectLight02->shadowIntensity = 0.6f;
    interiorRectLight02->shadowSoftness = 2.0f;
    {
        glm::vec3 localDir(0.0f, -1.0f, 0.0f);
        glm::vec3 localUp(0.0f, 0.0f, 1.0f);
        interiorRectLight02->direction = glm::normalize(localDir);
        interiorRectLight02->up = glm::normalize(localUp);
        interiorRectLight02->right = glm::normalize(glm::cross(interiorRectLight02->direction, interiorRectLight02->up));
        interiorRectLight02->up = glm::normalize(glm::cross(interiorRectLight02->right, interiorRectLight02->direction));
    }

    Light* interiorPointLight01 = interiorScene->addPointLight("Interior Point Light 01");
    interiorPointLight01->position = glm::vec3(-0.037077f, 2.49269f, 5.7212f);
    interiorPointLight01->intensity = 0.5f;
    interiorPointLight01->color = glm::vec3(1.0f, 0.95f, 0.9f);
    interiorPointLight01->temperature = 4500.0f;
    interiorPointLight01->radius = 3.0f;
    interiorPointLight01->setCastShadows(false);
    interiorPointLight01->setActive(true);

    Light* interiorPointLight02 = interiorScene->addPointLight("Interior Point Light 02");
    interiorPointLight02->position = glm::vec3(-0.037077f, 2.49269f, 2.96778f);
    interiorPointLight02->intensity = 0.5f;
    interiorPointLight02->color = glm::vec3(1.0f, 0.95f, 0.9f);
    interiorPointLight02->temperature = 4500.0f;
    interiorPointLight02->radius = 3.0f;
    interiorPointLight02->setCastShadows(false);
    interiorPointLight02->setActive(true);

    Light* kitchenRectLight = interiorScene->addRectLight("Kitchen Rect Light");
    kitchenRectLight->position = glm::vec3(-3.36698f, 2.49269f, 3.65093f);
    kitchenRectLight->intensity = 15.0f/1.5f;
    kitchenRectLight->color = glm::vec3(1.0f, 0.95f, 0.9f);
    kitchenRectLight->temperature = 5000.0f;
    kitchenRectLight->setRectLight(1.0f, 1.0f);
    kitchenRectLight->setCastShadows(true);
    kitchenRectLight->shadowIntensity = 0.6f;
    kitchenRectLight->shadowSoftness = 2.0f;
    {
        glm::vec3 localDir(0.0f, -1.0f, 0.0f);
        glm::vec3 localUp(0.0f, 0.0f, 1.0f);
        kitchenRectLight->direction = glm::normalize(localDir);
        kitchenRectLight->up = glm::normalize(localUp);
        kitchenRectLight->right = glm::normalize(glm::cross(kitchenRectLight->direction, kitchenRectLight->up));
        kitchenRectLight->up = glm::normalize(glm::cross(kitchenRectLight->right, kitchenRectLight->direction));
    }

    Light* kitchenPointLight = interiorScene->addPointLight("Kitchen Point Light");
    kitchenPointLight->position = glm::vec3(-3.36698f, 2.49269f, 3.65093f);
    kitchenPointLight->intensity = 0.5f/1.5f;
    kitchenPointLight->color = glm::vec3(1.0f, 0.95f, 0.9f);
    kitchenPointLight->temperature = 4500.0f;
    kitchenPointLight->radius = 3.0f;
    kitchenPointLight->setCastShadows(false);
    kitchenPointLight->setActive(true);

    std::cout << "[Engine] Interior rect lights + point lights added to interiorScene." << std::endl;

    LightProbeManager* interiorProbeMgr = interiorScene->getLightProbeManager();
    interiorProbeMgr->setSpacing(0.5f);
    interiorProbeMgr->setNumBounces(5);

    std::string interiorProbePath = "../content/light_probes/interior.pak";
    if (!interiorProbeMgr->loadFromCache(interiorProbePath)) {
        std::cout << "[Engine] Baking interior light probes..." << std::endl;
        PhysicsWorld interiorPhysics;
        for (SceneObject* obj : interiorScene->getObjects()) {
            if (obj->isStatic() && obj->isVisible())
                interiorPhysics.addStaticMeshCollider(obj);
        }
        interiorProbeMgr->bakeAndSave(&interiorPhysics, renderer.get(),
                                      "../content/hdri/NightSky4K.hdr",
                                      interiorProbePath);
        std::cout << "[Engine] Interior light probes baked and saved." << std::endl;
    }
    interiorProbeMgr->createDebugObjects();
    interiorScene->setAmbientColor(glm::vec3(0.20, 0.14, 0.08));


    prologueDoorAnimation = ModelLoader::loadAnimation(
        "../content/animations/prologue/Prologue_door_animation.fbx"
    );
    if (prologueDoorAnimation) {
        prologueDoorAnimation->setLooping(false);
        std::cout << "Prologue door animation loaded (will start after camera animation)." << std::endl;
        for (const auto& ch : prologueDoorAnimation->getChannels()) {
            std::cout << "Channel: '" << ch.boneName << "'" 
                      << " (Rot keys: " << ch.rotationKeys.size() << ")" << std::endl;
        }
    } else {
        std::cerr << "Failed to load prologue door animation!" << std::endl;
    }

    prologueCharacterAnimation = ModelLoader::loadAnimation(
        "../content/animations/prologue/Prologue_character_enter.fbx"
    );
    if (prologueCharacterAnimation) {
        prologueCharacterAnimation->setLooping(false);
        prologueCharacterAnimation->setAnimationSpeed(1.0f);
        std::cout << "Prologue character animation loaded (will start with door)." << std::endl;
    } else {
        std::cerr << "Failed to load prologue character animation!" << std::endl;
    }

    prologueHandleAnimation = ModelLoader::loadAnimation(
        "../content/animations/prologue/Prologue_handle_animation.fbx"
    );
    if (prologueHandleAnimation) {
        prologueHandleAnimation->setLooping(false);
        std::cout << "Prologue handle animation loaded." << std::endl;
        for (int i = 0; i < door01Model->getMeshCount(); i++) {
            Mesh* mesh = door01Model->getMesh(i);
            if (mesh && mesh->name == "Circle.001") {
                doorHandleMesh = mesh;
                std::cout << "Found door handle mesh: " << mesh->name << std::endl;
                break;
            }
        }
        if (!doorHandleMesh) {
            std::cerr << "Warning: Could not find mesh 'Circle.001' in door model." << std::endl;
        }
    } else {
        std::cerr << "Failed to load prologue handle animation!" << std::endl;
    }

    if (door01Model) {
        for (int i = 0; i < door01Model->getMeshCount(); i++) {
            Mesh* mesh = door01Model->getMesh(i);
            if (mesh) mesh->material.invertNormals = true;
            mesh->material.ssaoMultiplier = 0.8f;
        }
    }
    if (doorFrame01Model) {
        for (int i = 0; i < doorFrame01Model->getMeshCount(); i++) {
            Mesh* mesh = doorFrame01Model->getMesh(i);
            if (mesh) mesh->material.invertNormals = true;
            mesh->material.ssaoMultiplier = 0.8f;
        }
    }

    if (characterModel) {
        std::vector<Bone> skeleton = ModelLoader::loadSkeleton(
            "../content/character/main/anim_walking.fbx"
        );
        if (!skeleton.empty()) {
            std::cout << "Character skeleton loaded with " << skeleton.size() << " bones" << std::endl;
            characterModel->setSkeleton(skeleton);
            characterModel->setShowBones(true);

            idleAnimation = ModelLoader::loadAnimation("../content/character/main/anim_idle.fbx");
            walkAnimation = ModelLoader::loadAnimation("../content/character/main/anim_walking.fbx");

            if (idleAnimation) {
                idleAnimation->setLooping(true);
                idleAnimation->play();
                std::cout << "Idle animation loaded." << std::endl;
            } else {
                std::cerr << "Failed to load idle animation!" << std::endl;
            }
            if (walkAnimation) {
                walkAnimation->setLooping(true);
                walkAnimation->play();
                std::cout << "Walk animation loaded." << std::endl;
            } else {
                std::cerr << "Failed to load walk animation!" << std::endl;
            }

            animationManager = std::make_unique<AnimationManager>();
            if (walkAnimation) {
                animationManager->setAnimation(walkAnimation, characterModel);
                std::cout << "Character walk animation set for cutscene." << std::endl;
            }
        } else {
            std::cerr << "Failed to load character skeleton!" << std::endl;
        }
    }

    if (characterModel && characterModel->getMeshCount() > 0) {
        characterObjectPtr = std::make_unique<SceneObject>("Character", nullptr, characterModel);
        characterObject = characterObjectPtr.get();
        characterObject->position = glm::vec3(0.0f, 0.0f, -2.0f);
        characterObject->scale = glm::vec3(0.004818f, 0.004818f, 0.004818f);
        characterObject->setVisible(false);
        std::cout << "[Engine] Character object created independently. Hidden initially." << std::endl;
    } else {
        std::cerr << "[Engine] Failed to create character object!" << std::endl;
    }

    Scene* scene = exteriorScene.get();

    SceneObject* grass01Obj_001 = scene->addObject(grass01Model, "Grass01_001");
    grass01Obj_001->position = glm::vec3(0.0f, 0.0f, 0.0f);
    SceneObject* grass01Obj_002 = scene->addObject(grass01Model, "Grass01_002");
    grass01Obj_002->position = glm::vec3(11.6854f, 0.0f, 0.0f);
    SceneObject* grass01Obj_003 = scene->addObject(grass01Model, "Grass01_003");
    grass01Obj_003->position = glm::vec3(11.6854f, 0, 12.3751f);
    SceneObject* grass01Obj_004 = scene->addObject(grass01Model, "Grass01_004");
    grass01Obj_004->position = glm::vec3(-11.6854f, 0.0f, 12.3751f);
    SceneObject* grass01Obj_005 = scene->addObject(grass01Model, "Grass01_005");
    grass01Obj_005->position = glm::vec3(-11.6854f, 0.0f, 0.0f);
    SceneObject* grass01Obj_006 = scene->addObject(grass01Model, "Grass01_006");
    grass01Obj_006->position = glm::vec3(0.0f, 0.0f, 12.3751f);

    SceneObject* streetLamp01Obj_001 = scene->addObject(streetLamp01Model, "StreetLamp01_001");
    streetLamp01Obj_001->position = glm::vec3(1.48683f, 0.0f, -3.99562f);
    SceneObject* streetLamp01Obj_002 = scene->addObject(streetLamp01Model, "StreetLamp01_002");
    streetLamp01Obj_002->position = glm::vec3(-1.43409f, 0.0f, -3.99562f);
    SceneObject* streetLamp01Obj_003 = scene->addObject(streetLamp01Model, "StreetLamp01_003");
    streetLamp01Obj_003->position = glm::vec3(1.48683f, 0.0f, -0.967409f);
    SceneObject* streetLamp01Obj_004 = scene->addObject(streetLamp01Model, "StreetLamp01_004");
    streetLamp01Obj_004->position = glm::vec3(-1.43409f, 0.0f, -0.967409f);
    SceneObject* streetLamp02Obj_001 = scene->addObject(streetLamp02Model, "StreetLamp02_001");
    streetLamp02Obj_001->position = glm::vec3(0.750496f, 1.95849f, 2.06928f);
    streetLamp02Obj_001->rotation = glm::vec3(0.0f, 180.0f, 0.0f);
    SceneObject* streetLamp02Obj_002 = scene->addObject(streetLamp02Model, "StreetLamp02_002");
    streetLamp02Obj_002->position = glm::vec3(-0.733575f, 1.95849f, 2.06928f);
    streetLamp02Obj_002->rotation = glm::vec3(0.0f, 180.0f, 0.0f);

    SceneObject* tree01Obj_001 = scene->addObject(tree01Model, "Tree01_001");
    tree01Obj_001->position = glm::vec3(3.56188f, 0.0f, -4.96321f);
    tree01Obj_001->scale = glm::vec3(0.000908f, 0.000908f, 0.000908f);
    tree01Obj_001->rotation = glm::vec3(180.0f, 86.57f, 0.0f);
    SceneObject* tree01Obj_002 = scene->addObject(tree01Model, "Tree01_002");
    tree01Obj_002->position = glm::vec3(-3.04871f, 0.0f, -4.96321f);
    tree01Obj_002->scale = glm::vec3(0.000975f, 0.000975f, 0.000975f);
    tree01Obj_002->rotation = glm::vec3(180.0f, 63.8495f, 0.0f);
    SceneObject* tree01Obj_003 = scene->addObject(tree01Model, "Tree01_003");
    tree01Obj_003->position = glm::vec3(6.71564f, 0.0f, -2.0302f);
    tree01Obj_003->scale = glm::vec3(0.001f, 0.001f, 0.001f);
    tree01Obj_003->rotation = glm::vec3(180.0f, 68.1583f, 0.0f);
    SceneObject* tree01Obj_004 = scene->addObject(tree01Model, "Tree01_004");
    tree01Obj_004->position = glm::vec3(2.8057f, 0.0f, -2.51396f);
    tree01Obj_004->scale = glm::vec3(0.001f, 0.001f, 0.001f);
    tree01Obj_004->rotation = glm::vec3(180.0f, 103.196f, 0.0f);
    SceneObject* tree01Obj_005 = scene->addObject(tree01Model, "Tree01_005");
    tree01Obj_005->position = glm::vec3(-3.50068f, 0.0f, -2.31802f);
    tree01Obj_005->scale = glm::vec3(0.001f, 0.001f, 0.001f);
    tree01Obj_005->rotation = glm::vec3(180.0f, 43.7582f, 0.0f);
    SceneObject* tree01Obj_006 = scene->addObject(tree01Model, "Tree01_006");
    tree01Obj_006->position = glm::vec3(6.87484f, 0.0f, 2.7581f);
    tree01Obj_006->scale = glm::vec3(0.001f, 0.001f, 0.001f);
    tree01Obj_006->rotation = glm::vec3(180.0f, 187.502f, 0.0f);
    SceneObject* tree01Obj_007 = scene->addObject(tree01Model, "Tree01_007");
    tree01Obj_007->position = glm::vec3(3.66732f, 0.0f, 0.53769f);
    tree01Obj_007->scale = glm::vec3(0.001f, 0.001f, 0.001f);
    tree01Obj_007->rotation = glm::vec3(180.0f, 50.0614f, 0.0f);
    SceneObject* tree02Obj_008 = scene->addObject(tree02Model, "Tree02_008");
    tree02Obj_008->position = glm::vec3(-3.73789f, 0.0f, 1.57885f);
    tree02Obj_008->scale = glm::vec3(0.001f, 0.001f, 0.001f);
    tree02Obj_008->rotation = glm::vec3(180.0f, 14.6261f+180, 0.0f);

    SceneObject* porchObj = scene->addObject(porchModel, "Porch");
    porchObj->position = glm::vec3(0.0f, 0.57738f, 4.20504f);

    SceneObject* houseExteriorObj = scene->addObject(houseExteriorModel, "HouseExterior");
    houseExteriorObj->position = glm::vec3(0.0f, 0.57738f, 4.20504f);

    SceneObject* walkwayObj = scene->addObject(walkwayModel, "Walkway");
    walkwayObj->position = glm::vec3(0.038907f, 0.0093f, -2.46824f);

    SceneObject* fence01GroupObj = scene->addObject(fence01GroupModel, "Fence01Group");
    fence01GroupObj->position = glm::vec3(0.0f, 0.0f, 0.0f);

    SceneObject* fog01GroupObj = scene->addObject(fog01GroupModel, "Fog01Group");
    fog01GroupObj->position = glm::vec3(0.0f, 0.0f, 0.0f);

    prologueDoor01Obj_001 = scene->addObject(door01Model, "Door01_001");
    prologueDoor01Obj_001->position = glm::vec3(-0.484459f, 1.85677f, 4.10808f);
    prologueDoor01Obj_001->scale = glm::vec3(1.1f, 1.1f, 1.1f);
    SceneObject* doorFrame01Obj_001 = scene->addObject(doorFrame01Model, "DoorFrame01_001");
    doorFrame01Obj_001->position = glm::vec3(0.0f, 0.558877f, 4.11115f);
    doorFrame01Obj_001->scale = glm::vec3(1.1f, 1.1f, 1.1f);


    if (fog01GroupModel) {
        static GLuint sharedAlphaTexture = 0;
        if (sharedAlphaTexture == 0) {
            unsigned char alphaData[4] = {255, 255, 255, 102};
            glGenTextures(1, &sharedAlphaTexture);
            glBindTexture(GL_TEXTURE_2D, sharedAlphaTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB_ALPHA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, alphaData);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glBindTexture(GL_TEXTURE_2D, 0);
            std::cout << "Created shared alpha texture for fog (ID: " << sharedAlphaTexture << ")" << std::endl;
        }
        for (int i = 0; i < fog01GroupModel->getMeshCount(); i++) {
            Mesh* mesh = fog01GroupModel->getMesh(i);
            if (mesh) {
                mesh->material.blendMode = true;
                mesh->material.albedoFactor = glm::vec3(0.1f, 0.1f, 0.1f);
                mesh->material.diffuseFactor = glm::vec3(0.1f, 0.1f, 0.1f);
                mesh->material.alphaTexture = sharedAlphaTexture;
                mesh->material.hasAlphaTexture = true;
                mesh->material.alphaTextureHasAlpha = true;
                mesh->material.alphaThreshold = 0.0f;
                mesh->material.roughness = 0.9f;
                mesh->material.metallic = 0.0f;
            }
        }
    }
    
    SceneObject* border01Obj = scene->addObject(borderO1Model, "Border01");
    border01Obj->position = glm::vec3(0.0f, 0.0f, -7.20682f);

    Light* streetLampLight01_001 = scene->addPointLight("Street Lamp Light 01 001");
    streetLampLight01_001->position = glm::vec3(1.48683f, 2.23442f, -3.99562f);
    streetLampLight01_001->intensity = 1.0f;
    streetLampLight01_001->shadowIntensity = 0.8f;
    streetLampLight01_001->shadowSoftness = 2.0f;
    streetLampLight01_001->temperature = 7000.0f;
    streetLampLight01_001->setShadowResolution(streetLampShadowResolution, streetLampShadowResolution);
    streetLampLight01_001->setCastShadows(false);

    Light* streetLampLight01_002 = scene->addPointLight("Street Lamp Light 01 002");
    streetLampLight01_002->position = glm::vec3(-1.43409f, 2.23442f, -3.99562f);
    streetLampLight01_002->intensity = 1.0f;
    streetLampLight01_002->shadowIntensity = 0.8f;
    streetLampLight01_002->shadowSoftness = 2.0f;
    streetLampLight01_002->temperature = 7000.0f;
    streetLampLight01_002->setShadowResolution(streetLampShadowResolution, streetLampShadowResolution);
    streetLampLight01_002->setCastShadows(false);

    Light* streetLampLight01_003 = scene->addPointLight("Street Lamp Light 01 003");
    streetLampLight01_003->position = glm::vec3(1.48683f, 2.23442f, -0.967409f);
    streetLampLight01_003->intensity = 1.0f;
    streetLampLight01_003->shadowIntensity = 0.8f;
    streetLampLight01_003->shadowSoftness = 2.0f;
    streetLampLight01_003->temperature = 7000.0f;
    streetLampLight01_003->setShadowResolution(streetLampShadowResolution, streetLampShadowResolution);
    streetLampLight01_003->setCastShadows(false);

    Light* streetLampLight01_004 = scene->addPointLight("Street Lamp Light 01 004");
    streetLampLight01_004->position = glm::vec3(-1.43409f, 2.23442f, -0.967409f);
    streetLampLight01_004->intensity = 1.0f;
    streetLampLight01_004->shadowIntensity = 0.8f;
    streetLampLight01_004->shadowSoftness = 2.0f;
    streetLampLight01_004->temperature = 7000.0f;
    streetLampLight01_004->setShadowResolution(streetLampShadowResolution, streetLampShadowResolution);
    streetLampLight01_004->setCastShadows(false);

    Light* streetLampLight02_001 = scene->addPointLight("Street Lamp Light 02 001");
    streetLampLight02_001->position = glm::vec3(0.750496f, 2.49187f, 1.63186f);
    streetLampLight02_001->intensity = 1.0f;
    streetLampLight02_001->shadowIntensity = 0.8f;
    streetLampLight02_001->shadowSoftness = 2.0f;
    streetLampLight02_001->temperature = 4500.0f;
    streetLampLight02_001->setShadowResolution(streetLampShadowResolution, streetLampShadowResolution);

    Light* streetLampLight02_002 = scene->addPointLight("Street Lamp Light 02 002");
    streetLampLight02_002->position = glm::vec3(-0.733575f, 2.49187f, 1.63186f);
    streetLampLight02_002->intensity = 1.0f;
    streetLampLight02_002->shadowIntensity = 0.8f;
    streetLampLight02_002->shadowSoftness = 2.0f;
    streetLampLight02_002->temperature = 4500.0f;
    streetLampLight02_002->setShadowResolution(streetLampShadowResolution, streetLampShadowResolution);

    Light* rectLight = scene->addRectLight("Rect Light");
    rectLight->position = glm::vec3(-0.344022f, 7.42728f, 1.51492f);
    rectLight->shadowIntensity = 0.2f;
    rectLight->shadowSoftness = 3.0f;
    rectLight->intensity = 0.03f;
    rectLight->color = glm::vec3(1.0f, 1.0f, 0.95f);
    rectLight->radius = 100.0f;
    rectLight->setShadowResolution(4096, 4096);

    float rotX = glm::radians(360 + (15.542f - 90.0f));
    float rotY = glm::radians(-8.67585f);
    float rotZ = glm::radians(-2.40225f);
    glm::vec3 localDir(0.0f, 0.0f, -1.0f);
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, rotZ, glm::vec3(0.0f, 0.0f, 1.0f));
    rotation = glm::rotate(rotation, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
    glm::vec3 worldDir = glm::vec3(rotation * glm::vec4(localDir, 0.0f));
    rectLight->direction = glm::normalize(worldDir);
    rectLight->setRectLight(11.0f, 11.0f);

    Light* doorLight01 = scene->addPointLight("Door Light 01");
    doorLight01->position = glm::vec3(0.0f, 2.77217f, 3.04847f);
    doorLight01->shadowIntensity = 0.6f;
    doorLight01->shadowSoftness = 3.0f;
    doorLight01->intensity = 1.0f;
    doorLight01->temperature = 4000;
    doorLight01->setShadowResolution(1024, 1024);

    Light* doorLight02 = scene->addRectLight("Door Light 02");
    doorLight02->position = glm::vec3(-0.21502f, 1.55207f, 3.16122f);
    doorLight02->intensity = 3.0f;
    doorLight02->temperature = 3500;
    doorLight02->setRectLight(1.0f, 1.0f);
    doorLight02->setCastShadows(true);
    doorLight02->setActive(false);
    {
        float rotX = glm::radians(95.7974f + 90.0f);
        float rotY = glm::radians(2.92482f);
        float rotZ = glm::radians(32.008f);
        glm::vec3 localDir(0.0f, 0.0f, -1.0f);
        glm::vec3 localUp (0.0f, 1.0f,  0.0f);
        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, rotZ, glm::vec3(0.0f, 0.0f, 1.0f));
        rotation = glm::rotate(rotation, rotY, glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, rotX, glm::vec3(1.0f, 0.0f, 0.0f));
        glm::vec3 worldDir = glm::vec3(rotation * glm::vec4(localDir, 0.0f));
        glm::vec3 worldUp  = glm::vec3(rotation * glm::vec4(localUp,  0.0f));
        doorLight02->direction = glm::normalize(worldDir);
        doorLight02->up        = glm::normalize(worldUp);
        doorLight02->right     = glm::normalize(glm::cross(doorLight02->direction, doorLight02->up));
        doorLight02->up        = glm::normalize(glm::cross(doorLight02->right, doorLight02->direction));
    }

    lightManager = std::make_unique<LightManager>(*currentScene);
    lightManager->addLight(rectLight, {1});
    lightManager->addLight(doorLight02, {2});
    lightManager->loadDirectorAnimation("../content/animations/prologue/Prologue_light_director_animation.fbx");
    lightManager->addTagChangeListener(2, [this](int oldTag, int newTag) {
        if (door01Model) {
            for (int i = 0; i < door01Model->getMeshCount(); ++i) {
                Mesh* mesh = door01Model->getMesh(i);
                if (mesh) mesh->material.ssaoMultiplier = 0.1f;
            }
        }
        if (doorFrame01Model) {
            for (int i = 0; i < doorFrame01Model->getMeshCount(); ++i) {
                Mesh* mesh = doorFrame01Model->getMesh(i);
                if (mesh) mesh->material.ssaoMultiplier = 0.1f;
            }
        }
        std::cout << "[LightManager] Tag 2 activated: SSAO multiplier set to 0.1 for door and frame" << std::endl;
    });

    physicsWorld = std::make_unique<PhysicsWorld>();
    if (characterObject) {
        float capsuleRadius = 0.3f;
        float capsuleHeight = 1.0f;
        btCollisionShape* capsule = PhysicsWorld::createCapsuleShape(capsuleRadius, capsuleHeight);
        glm::vec3 offsetPos(0.0f, capsuleHeight * 0.5f + capsuleRadius, 0.0f);
        glm::vec3 offsetRot(0.0f);
        glm::vec3 offsetScale(1.0f);
        glm::vec3 startPos = characterObject->position + offsetPos;
        Mesh* firstMesh = characterModel->getMesh(0);
        if (firstMesh) {
            firstMesh->collision = new CollisionComponent();
            firstMesh->collision->setOffsetPosition(offsetPos);
            firstMesh->collision->setOffsetRotation(offsetRot);
            firstMesh->collision->setOffsetScale(offsetScale);
            firstMesh->collision->createDynamic(capsule, startPos, 1.0f, 0.0f, 0.0f);
            firstMesh->collision->setSceneObject(characterObject);
            physicsWorld->addRigidBody(firstMesh->collision);
        }
    }
    for (SceneObject* obj : currentScene->getObjects()) {
        if (obj->isStatic() && obj->isVisible()) {
            physicsWorld->addStaticMeshCollider(obj);
        }
    }

    LightProbeManager* probeMgr = currentScene->getLightProbeManager();
    probeMgr->setBakingVolume(glm::vec3(-5, 0.1f, -5), glm::vec3(5, 5, 5));
    probeMgr->setSpacing(0.5f);
    probeMgr->setNumBounces(5);
    std::string probeCachePath = "../content/light_probes/prologue.pak";
    if (!probeMgr->loadFromCache(probeCachePath)) {
        std::cout << "[Engine] Baking exterior light probes..." << std::endl;
        probeMgr->bakeAndSave(physicsWorld.get(), renderer.get(),
                              "../content/hdri/NightSky4K.hdr",
                              probeCachePath);
        std::cout << "[Engine] Exterior light probes baked and saved." << std::endl;
    }
    GLuint ssbo = probeMgr->getProbeSSBO();
    if (ssbo) {
        renderer->setLightProbeSSBO(ssbo,
                                    probeMgr->getProbeGridResolution(),
                                    probeMgr->getProbeGridMin(),
                                    probeMgr->getProbeGridSize());
    }
    probeMgr->createDebugObjects();

    PostSettings exteriorPost;
    exteriorPost.bloomEnabled = false;
    exteriorPost.bloomIntensity = 2.0f;
    exteriorPost.bloomBlurSize = 1.0f;
    exteriorPost.blurEnabled = false;
    exteriorPost.blurSize = 0.5f;
    exteriorPost.dofEnabled = false;
    exteriorPost.dofFocusDistance = 5.0f;
    exteriorPost.dofAperture = 0.5f;
    exteriorPost.dofMaxBlur = 10.0f;
    exteriorPost.aaMode = FXAA;
    exteriorPost.colorGradingEnabled = false;
    exteriorPost.saturation = 1.0f;
    exteriorPost.grayscale = 0.5f;
    exteriorPost.ssaoEnabled = true;
    exteriorPost.volumetricFogEnabled = true;
    exteriorScene->setPostSettings(exteriorPost);

    PostSettings interiorPost;
    interiorPost.bloomEnabled = false;
    interiorPost.bloomIntensity = 2.0f;
    interiorPost.bloomBlurSize = 1.0f;
    interiorPost.blurEnabled = false;
    interiorPost.blurSize = 0.5f;
    interiorPost.dofEnabled = false;
    interiorPost.dofFocusDistance = 5.0f;
    interiorPost.dofAperture = 0.5f;
    interiorPost.dofMaxBlur = 10.0f;
    interiorPost.aaMode = FXAA;
    interiorPost.colorGradingEnabled = true;
    interiorPost.saturation = 0.7f;
    interiorPost.grayscale = 0.1f;
    interiorPost.ssaoEnabled = true;
    interiorPost.volumetricFogEnabled = true;
    interiorScene->setPostSettings(interiorPost);

    renderer->setFogColor(glm::vec3(0.72, 0.78, 0.83));
    renderer->setFogDensity(0.015f);
    renderer->setFogHeightFalloff(0.2f);
    renderer->setVolumetricFogSteps(64);

    cameraAnimation = ModelLoader::loadAnimation("../content/animations/prologue/Prologue_animation.fbx");
    if (cameraAnimation) {
        cameraAnimation->setLooping(false);
        cameraAnimation->play();
        cameraAnimation->setAnimationSpeed(0.9f);
        std::cout << "Camera animation loaded and playing." << std::endl;
        float animDuration = cameraAnimation->getDuration() / cameraAnimation->getAnimationSpeed();
        cameraShake.start(
            glm::vec3(0.008f, 0.004f, 0.008f),
            glm::vec3(0.002f, 0.003f, 0.001f),
            1.5f,
            animDuration
        );
    } else {
        std::cerr << "Failed to load camera animation!" << std::endl;
    }

    characterTransformAnimation = ModelLoader::loadAnimation(
        "../content/animations/prologue/Prologue_animation_character.fbx"
    );
    if (characterTransformAnimation) {
        characterTransformAnimation->setLooping(false);
        characterTransformAnimation->play();
        characterTransformAnimation->setAnimationSpeed(0.9f);
        std::cout << "Character transform animation loaded and playing." << std::endl;
        startCutscene(characterObject);
    } else {
        std::cerr << "Failed to load character transform animation!" << std::endl;
    }

    fadeOverlay = std::make_unique<UIFadeOverlay>();
    fadeOverlay->setScreenSize(screenWidth, screenHeight);

    audioManager = std::make_unique<AudioManager>();
    if (audioManager->init()) {
        std::cout << "[Engine] Audio system initialized." << std::endl;
        
        ambientBuffer = audioManager->loadSound("../content/sounds/ambient.wav");
        if (ambientBuffer) {
            ambientSource = audioManager->create2DSource(ambientBuffer, 0.05f, true);
        }
        
        dogPantingBuffer = audioManager->loadSound("../content/sounds/dog_panting.wav");
        
        const std::vector<std::string> footstepFiles = {
            "../content/sounds/footsteps_01.wav",
            "../content/sounds/footsteps_02.wav",
            "../content/sounds/footsteps_03.wav",
            "../content/sounds/footsteps_04.wav"
        };
        for (const auto& path : footstepFiles) {
            ALuint buf = audioManager->loadSound(path);
            if (buf) {
                footstepBuffers.push_back(buf);
                std::cout << "[Audio] Loaded footstep sound: " << path << std::endl;
            } else {
                std::cerr << "[Audio] Failed to load: " << path << std::endl;
            }
        }

        if (!footstepBuffers.empty()) {
            footstepSourceLeft = audioManager->create2DSource(footstepBuffers[0], 0.15f, false);
            footstepSourceRight = audioManager->create2DSource(footstepBuffers[0], 0.15f, false);
            std::cout << "[Audio] Footstep sources created (Left: " << footstepSourceLeft 
                      << ", Right: " << footstepSourceRight << ")" << std::endl;
        } else {
            std::cerr << "[Audio] No footstep sounds loaded!" << std::endl;
        }
    } else {
        std::cerr << "[Engine] Audio system failed to initialize!" << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    GLenum err = glGetError();
    while (err != GL_NO_ERROR) {
        std::cerr << "OpenGL error before first frame: 0x" << std::hex << err << std::dec << std::endl;
        err = glGetError();
    }
    lastFrame = glfwGetTime(); 

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        static int frameCounter = 0;
        frameCounter++;

        if (frameCounter % 60 == 0) {
            GLint drawCalls = 0;
            GLint triangleCount = 0;
            auto allObjects = currentScene->getAllObjects();
            if (characterObject && characterObject->isVisible()) {
                allObjects.push_back(characterObject);
            }
            for (SceneObject* obj : allObjects) {
                if (!obj->isVisible()) continue;
                if (obj->model) {
                    drawCalls += obj->model->getMeshCount();
                    for (int i = 0; i < obj->model->getMeshCount(); i++) {
                        Mesh* mesh = obj->model->getMesh(i);
                        if (mesh) triangleCount += mesh->getIndices().size() / 3;
                    }
                } else if (obj->mesh) {
                    drawCalls++;
                    triangleCount += obj->mesh->getIndices().size() / 3;
                }
            }
            std::cout << "Draw calls: " << drawCalls 
                    << " | Triangles: " << triangleCount 
                    << " | Lights: " << currentScene->getLights().size()
                    << " | Objects: " << allObjects.size() << std::endl;
        }

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        if (animationManager && !characterControlEnabled) {
            animationManager->update(deltaTime);
        }

        if (prologueCharacterAnimation && prologueCharacterAnimationStarted && !qteCompleted) {
            float currentTime = prologueCharacterAnimation->getCurrentTime();
            if (!qteActive && currentTime >= 24.0f) {
                prologueCharacterAnimation->setAnimationSpeed(0.0f);
                if (prologueDoorAnimation) prologueDoorAnimation->setAnimationSpeed(0.0f);
                if (prologueHandleAnimation) prologueHandleAnimation->setAnimationSpeed(0.0f);
                qteActive = true;
                qteProgress = 0.0f;
                if (qte) qte->setActive(true);
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                std::cout << "[QTE] Started at frame 24" << std::endl;
            }
            if (qteActive) {
                if (qte) {
                    qte->update(deltaTime, window, 0);
                    qteProgress = qte->getProgress();
                    float animTime = 24.0f + qteProgress * (40.0f - 24.0f);
                    prologueCharacterAnimation->setCurrentTime(animTime);
                    if (prologueDoorAnimation) prologueDoorAnimation->setCurrentTime(animTime);
                    if (prologueHandleAnimation) prologueHandleAnimation->setCurrentTime(animTime);
                }
                if (qteProgress >= 1.0f) {
                    qteCompleted = true;
                    qteActive = false;
                    if (qte) qte->setActive(false);
                    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                    prologueCharacterAnimation->setCurrentTime(40.0f);
                    prologueCharacterAnimation->setAnimationSpeed(1.0f);
                    if (prologueDoorAnimation) {
                        prologueDoorAnimation->setCurrentTime(40.0f);
                        prologueDoorAnimation->setAnimationSpeed(1.0f);
                    }
                    if (prologueHandleAnimation) {
                        prologueHandleAnimation->setCurrentTime(40.0f);
                        prologueHandleAnimation->setAnimationSpeed(1.0f);
                    }
                    std::cout << "[QTE] Completed, animation resumed from frame 40" << std::endl;
                }
            }
        }

        if (prologueCharacterAnimationStarted && prologueCharacterAnimation &&
            prologueCharacterAnimation->isPlaying() && !transitionToInteriorDone) {
            float charTime = prologueCharacterAnimation->getCurrentTime();
            float animDuration = prologueCharacterAnimation->getDuration();
            float transitionFrame = animDuration - 8.0f;
            if (charTime >= transitionFrame) {
                prologueCharacterAnimation->stop();
                if (prologueDoorAnimation) prologueDoorAnimation->stop();
                if (prologueHandleAnimation) prologueHandleAnimation->stop();
                std::cout << "[Engine] Character animation stopped at frame " << charTime << std::endl;
                if (characterObject && characterObject->isVisible()) {
                    characterObject->setVisible(false);
                }
                switchToScene(interiorScene.get());
                cleanCameraPosition = glm::vec3(0.0f, 1.41403f, 2.32021f);
                float yawAngle = glm::radians(0.0f+90.0f);
                float pitchAngle = glm::radians(-6.0f);
                glm::vec3 newFront;
                newFront.x = cos(yawAngle) * cos(pitchAngle);
                newFront.y = sin(pitchAngle);
                newFront.z = sin(yawAngle) * cos(pitchAngle);
                cleanCameraFront = glm::normalize(newFront);
                glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
                cleanCameraRight = glm::normalize(glm::cross(cleanCameraFront, worldUp));
                cleanCameraUp = glm::normalize(glm::cross(cleanCameraRight, cleanCameraFront));
                camera.position = cleanCameraPosition;
                camera.front = cleanCameraFront;
                camera.up = cleanCameraUp;
                camera.right = cleanCameraRight;
                endCutscene();
                if (characterObject && characterModel && idleAnimation && walkAnimation) {
                    characterObject->setVisible(true);
                    characterObject->setSyncRotationFromPhysics(false);
                    glm::vec3 targetPos(0.0f, 0.0f, 1.3f);
                    characterObject->position = targetPos;
                    characterObject->rotation = glm::vec3(0.0f, 0.0f, 0.0f);

                    if (characterModel->getMesh(0) && characterModel->getMesh(0)->collision) {
                        btRigidBody* body = characterModel->getMesh(0)->collision->getBody();
                        if (body) {
                            glm::vec3 offsetPos = characterModel->getMesh(0)->collision->getOffsetPosition();
                            btTransform trans;
                            trans.setIdentity();
                            trans.setOrigin(btVector3(targetPos.x + offsetPos.x,
                                                    targetPos.y + offsetPos.y,
                                                    targetPos.z + offsetPos.z));
                            btQuaternion rot;
                            rot.setRotation(btVector3(0, 1, 0), glm::radians(characterObject->rotation.y));
                            trans.setRotation(rot);
                            body->setWorldTransform(trans);
                            body->setLinearVelocity(btVector3(0, 0, 0));
                            body->setAngularVelocity(btVector3(0, 0, 0));
                            body->clearForces();
                            body->setActivationState(DISABLE_DEACTIVATION);
                            body->activate(true);

                            physicsWorld->stepSimulation(1.0f / 60.0f);

                            btTransform updatedTrans = body->getWorldTransform();
                            btVector3 updatedPos = updatedTrans.getOrigin();
                            characterObject->position = glm::vec3(updatedPos.x(), updatedPos.y(), updatedPos.z()) - offsetPos;
                        }
                    }

                    characterBlender = std::make_unique<AnimationBlender>();
                    characterBlender->setSource(idleAnimation, 1.0f);
                    characterBlender->setSource(walkAnimation, 0.0f);
                    characterControlEnabled = true;
                    currentCharacterSpeed = 0.0f;
                    characterJustTeleported = false;
                    std::cout << "[Engine] Character control enabled. Position: " 
                              << targetPos.x << ", " << targetPos.y << ", " << targetPos.z << std::endl;
                }
                transitionToInteriorDone = true;
                interiorEntryTime = glfwGetTime();
                thirdPerson2SecPassed = false;
            }
        }

        if (prologueCharacterAnimationStarted && prologueCharacterAnimation &&
            prologueCharacterAnimation->isPlaying() && !keyHiddenAndTeleported) {
            float charTime = prologueCharacterAnimation->getCurrentTime();
            if (charTime >= 69.0f) {
                if (keyObject && keyObject->isVisible()) {
                    keyObject->setVisible(false);
                }
                if (cutsceneCharacter) {
                    cutsceneCharacter->position = glm::vec3(0.540864f + 0.0375f - 0.02f, 0.478681f - 0.02f, 3.51361f - 0.03f + 0.02f + 0.02f);
                }
                keyHiddenAndTeleported = true;
            }
        }
        
        if (keyObject) {
            updateKeyAttachment();
        }
        
        if (prologueDoorAnimation && prologueDoorAnimation->isPlaying() && prologueDoor01Obj_001) {
            prologueDoorAnimation->update(deltaTime);
            const AnimationChannel* channel = prologueDoorAnimation->getChannel("Door");
            if (!channel) {
                for (const auto& ch : prologueDoorAnimation->getChannels()) {
                    if (!ch.rotationKeys.empty()) { channel = &ch; break; }
                }
            }
            if (channel && !channel->rotationKeys.empty()) {
                float animTime = prologueDoorAnimation->getCurrentTime();
                glm::quat rotation = channel->interpolateRotation(animTime);
                glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
                euler.x += 270.0f;
                euler.y = -euler.y;
                euler.z = 0.0f;
                prologueDoor01Obj_001->rotation = euler;
            }
        }

        if (prologueHandleAnimation && prologueHandleAnimation->isPlaying() && doorHandleMesh) {
            prologueHandleAnimation->update(deltaTime);
            const AnimationChannel* handleChannel = prologueHandleAnimation->getChannel("Circle.001");
            if (!handleChannel) {
                for (const auto& ch : prologueHandleAnimation->getChannels()) {
                    if (!ch.positionKeys.empty() || !ch.rotationKeys.empty()) { handleChannel = &ch; break; }
                }
            }
            if (handleChannel) {
                float animTime = prologueHandleAnimation->getCurrentTime();
                if (!handleChannel->rotationKeys.empty()) {
                    glm::quat rot = handleChannel->interpolateRotation(animTime);
                    glm::vec3 euler = glm::degrees(glm::eulerAngles(rot));
                    euler.y = euler.z-180.0f;
                    euler.z = 0.0f;
                    doorHandleMesh->setRotation(euler);
                }
            }
        }

        if (cutsceneActive && characterTransformAnimation && characterTransformAnimation->isPlaying()) {
            characterTransformAnimation->update(deltaTime);
            if (cutsceneCharacter) {
                const AnimationChannel* channel = characterTransformAnimation->getChannel("Character");
                if (!channel && !characterTransformAnimation->getChannels().empty()) {
                    channel = &characterTransformAnimation->getChannels()[0];
                }
                if (channel) {
                    float animTime = characterTransformAnimation->getCurrentTime();
                    if (!channel->positionKeys.empty()) {
                        const KeyFrame* closestKey = &channel->positionKeys[0];
                        float minDist = std::abs(animTime - closestKey->time);
                        for (const auto& key : channel->positionKeys) {
                            float dist = std::abs(animTime - key.time);
                            if (dist < minDist) { minDist = dist; closestKey = &key; }
                        }
                        cutsceneCharacter->position = closestKey->value.position;
                    }
                }
            }
        }

        if (cameraAnimation && cameraAnimation->isPlaying() && !freeCamera && !thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
            cameraAnimation->update(deltaTime);
            float currentTime = cameraAnimation->getCurrentTime();
            if (cutsceneCharacter && !cutsceneCharacter->isVisible() && currentTime >= 181.0f) {
                cutsceneCharacter->setVisible(true);
            }
            const AnimationChannel* camChannel = cameraAnimation->getChannel("Camera");
            if (camChannel) {
                glm::vec3 pos = cleanCameraPosition;
                glm::quat rot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
                if (!camChannel->positionKeys.empty()) {
                    const KeyFrame* closestKey = &camChannel->positionKeys[0];
                    float minDist = std::abs(currentTime - closestKey->time);
                    for (const auto& key : camChannel->positionKeys) {
                        float dist = std::abs(currentTime - key.time);
                        if (dist < minDist) { minDist = dist; closestKey = &key; }
                    }
                    pos = closestKey->value.position;
                }
                if (!camChannel->rotationKeys.empty()) {
                    const KeyFrame* closestKey = &camChannel->rotationKeys[0];
                    float minDist = std::abs(currentTime - closestKey->time);
                    for (const auto& key : camChannel->rotationKeys) {
                        float dist = std::abs(currentTime - key.time);
                        if (dist < minDist) { minDist = dist; closestKey = &key; }
                    }
                    rot = closestKey->value.rotation;
                }
                cleanCameraPosition = pos;
                glm::mat4 rotMatrix = glm::mat4_cast(rot);
                cleanCameraFront = glm::normalize(glm::vec3(rotMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f)));
                cleanCameraUp    = glm::normalize(glm::vec3(rotMatrix * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
                cleanCameraRight = glm::normalize(glm::cross(cleanCameraFront, cleanCameraUp));
            }
        }

        if (!thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
            if (!freeCamera) {
                camera.position = cleanCameraPosition;
                camera.front   = cleanCameraFront;
                camera.up      = cleanCameraUp;
                camera.right   = cleanCameraRight;
            } else {
                if (!freeCameraInitialized) {
                    camera.position = cleanCameraPosition;
                    camera.front    = cleanCameraFront;
                    camera.up       = cleanCameraUp;
                    camera.right    = cleanCameraRight;
                    freeCameraInitialized = true;
                }
            }
        }

        if (transitionToInteriorDone && !thirdPersonCameraActive && !thirdPersonTransitioning && !thirdPerson2SecPassed) {
            if (glfwGetTime() - interiorEntryTime >= 2.0) {
                thirdPerson2SecPassed = true;
                thirdPersonTransitioning = true;
                thirdPersonTransitionStartTime = glfwGetTime();
                thirdPersonTransitionStartPos = camera.position;
                thirdPersonTransitionStartFront = camera.front;
                glm::vec3 characterPos = characterObject->position;
                float yawRad = glm::radians(90.0f);
                float pitchRad = glm::radians(-30.0f);
                glm::vec3 front;
                front.x = cos(yawRad) * cos(pitchRad);
                front.y = sin(pitchRad);
                front.z = sin(yawRad) * cos(pitchRad);
                thirdPersonTargetFront = glm::normalize(front);
                thirdPersonTargetPos = characterPos + glm::vec3(0.0f, eyeHeight, 0.0f) 
                    - thirdPersonTargetFront * thirdPersonDistance 
                    + glm::normalize(glm::cross(thirdPersonTargetFront, glm::vec3(0.0f, 1.0f, 0.0f))) * thirdPersonRightOffset;
                camera.yaw = 90.0f;
                camera.pitch = -30.0f;
            }
        }

        if (thirdPersonTransitioning) {
            float elapsed = glfwGetTime() - thirdPersonTransitionStartTime;
            const float duration = 1.0f;
            if (elapsed >= duration) {
                thirdPersonTransitioning = false;
                thirdPersonCameraActive = true;
                camera.position = thirdPersonTargetPos;
                camera.front   = thirdPersonTargetFront;
                camera.right   = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
                camera.up      = glm::normalize(glm::cross(camera.right, camera.front));
            } else {
                float t = elapsed / duration;
                t = t * t * (3.0f - 2.0f * t);
                camera.position = glm::mix(thirdPersonTransitionStartPos, thirdPersonTargetPos, t);
                camera.front   = glm::normalize(glm::mix(thirdPersonTransitionStartFront, thirdPersonTargetFront, t));
                camera.right   = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
                camera.up      = glm::normalize(glm::cross(camera.right, camera.front));
            }
        }

        if (freeCamera) {
            inputManager->processInput(window, deltaTime);
        } else if (!cutsceneActive && !thirdPersonCameraActive && !thirdPersonTransitioning && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
            inputManager->processInput(window, deltaTime);
        }

        if (!freeCamera && cameraShake.isActive() && !thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
            ShakeOffset shake = cameraShake.update(deltaTime);
            camera.position = cleanCameraPosition + shake.positionOffset;
            if (glm::length(shake.rotationOffset) > 0.0001f) {
                glm::quat quatShake =
                    glm::angleAxis(shake.rotationOffset.x, cleanCameraRight) *
                    glm::angleAxis(shake.rotationOffset.y, cleanCameraUp) *
                    glm::angleAxis(shake.rotationOffset.z, cleanCameraFront);
                glm::mat3 rotMat = glm::mat3_cast(quatShake);
                camera.front = glm::normalize(rotMat * cleanCameraFront);
                camera.up    = glm::normalize(rotMat * cleanCameraUp);
                camera.right = glm::normalize(glm::cross(camera.front, camera.up));
            } else {
                camera.front = cleanCameraFront;
                camera.up    = cleanCameraUp;
                camera.right = cleanCameraRight;
            }
        }

        if (cameraAnimation && !cameraAnimation->isPlaying() && 
            prologueDoorAnimation && !prologueDoorAnimation->isPlaying() && 
            !prologueDoorAnimationStarted) {
            if (characterTransformAnimation) characterTransformAnimation->stop();
            if (cutsceneCharacter) {
                cutsceneCharacter->position = glm::vec3(0.540864f+0.0375f, 0.478681f-0.02f, 3.51361f-0.03f+0.02f);
            }
            keyModel = ModelLoader::loadFBX("../content/models/Key_01/models/Key_01.fbx");
            if (keyModel) {
                keyObject = currentScene->addObject(keyModel, "Key");
            }
            if (!qte) {
                qte = std::make_unique<QTE>('E', *fontManager, "../content/ui/fonts/Arial.ttf", 48);
                qte->setMode(QTE_MODE_SWIPE, 100.0f);
                qte->setSwipeDirection(QTESwipeDirection::RIGHT);
                qte->setSwipeRequiredPixels(150.0f);
                qte->setFillColor(glm::vec4(0.1f, 0.1f, 0.1f, 0.9f));
                qte->setOutlineColor(glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                qte->setOutlineThickness(3);
                qte->setCornerLength(0.15f);
                qte->setFontSizeFraction(0.5f);
                qte->setTextColor(glm::vec3(1.0f, 1.0f, 1.0f));
                qte->setCircleColor(glm::vec4(1.0f, 1.0f, 1.0f, 0.8f));
                qte->setCircleRadiusFraction(0.06f);
                qte->setArrowSizeFraction(0.35f);
                qte->setArrowOffsetFraction(0.25f);
                qte->setActive(false);
            }
            prologueDoorAnimation->play();
            prologueDoorAnimationStarted = true;
            if (prologueCharacterAnimation && !prologueCharacterAnimationStarted) {
                prologueCharacterAnimation->addBoneNameMapping("Armature", "Armature.001");
                prologueCharacterAnimation->play();
                animationManager->setAnimation(prologueCharacterAnimation, characterModel);
                animationManager->update(0.0f);
                prologueCharacterAnimationStarted = true;
            }
            if (prologueHandleAnimation && !prologueHandleAnimation->isPlaying()) {
                prologueHandleAnimation->play();
            }
        }

        if (cutsceneActive) {
            bool transformFinished = !characterTransformAnimation || !characterTransformAnimation->isPlaying();
            if (transformFinished) {
                bool characterAnimFinished = true;
                if (prologueCharacterAnimationStarted && prologueCharacterAnimation) {
                    characterAnimFinished = !prologueCharacterAnimation->isPlaying();
                }
                if (characterAnimFinished) endCutscene();
            }
        }

        if (characterControlEnabled && !cutsceneActive && characterObject && characterModel && thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
            glm::vec3 camForward = camera.front;
            camForward.y = 0.0f;
            if (glm::length(camForward) < 0.001f) camForward = glm::vec3(0.0f, 0.0f, -1.0f);
            camForward = glm::normalize(camForward);
            glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
            glm::vec3 camRight = glm::normalize(glm::cross(camForward, worldUp));
            glm::vec3 moveDir(0.0f);
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) moveDir += camForward;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) moveDir -= camForward;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) moveDir -= camRight;
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) moveDir += camRight;
            float targetSpeed = 0.0f;
            if (glm::length(moveDir) > 0.01f) { moveDir = glm::normalize(moveDir); targetSpeed = maxCharacterSpeed; }
            Mesh* firstMesh = characterModel->getMesh(0);
            if (firstMesh && firstMesh->collision) {
                btRigidBody* body = firstMesh->collision->getBody();
                if (body) {
                    body->setLinearVelocity(btVector3(moveDir.x * targetSpeed, 0.0f, moveDir.z * targetSpeed));
                    body->activate(true);
                }
            }
            float smoothing = 1.0f - exp(-deltaTime * 10.0f);
            currentCharacterSpeed += (targetSpeed - currentCharacterSpeed) * smoothing;
            float walkWeight = glm::clamp(currentCharacterSpeed / maxCharacterSpeed, 0.0f, 1.0f);
            float idleWeight = 1.0f - walkWeight;
            if (characterBlender) {
                characterBlender->clear();
                characterBlender->setSource(idleAnimation, idleWeight);
                characterBlender->setSource(walkAnimation, walkWeight);
                characterBlender->updateAnimations(deltaTime);
                characterBlender->applyToSkeleton(characterModel->getSkeleton(), characterObject->getFinalModelMatrix(), characterObject->scale);

                if (walkWeight > 0.5f && !footstepBuffers.empty() && audioManager) {
                    handleFootsteps(deltaTime);
                }
            }
            if (targetSpeed > 0.01f) {
                float moveYaw = std::atan2(moveDir.x, moveDir.z);
                float currentYaw = glm::radians(characterObject->rotation.y);
                float diff = moveYaw - currentYaw;
                if (diff > glm::radians(180.0f)) diff -= glm::radians(360.0f);
                if (diff < glm::radians(-180.0f)) diff += glm::radians(360.0f);
                float rotationSpeed = 15.0f;
                float t = glm::smoothstep(0.0f, 1.0f, rotationSpeed * deltaTime);
                float newYaw = currentYaw + diff * t;
                characterObject->rotation.y = glm::degrees(newYaw);
                if (firstMesh && firstMesh->collision) {
                    btRigidBody* body = firstMesh->collision->getBody();
                    if (body) {
                        btTransform trans = body->getWorldTransform();
                        btQuaternion rot;
                        rot.setRotation(btVector3(0, 1, 0), newYaw);
                        trans.setRotation(rot);
                        body->setWorldTransform(trans);
                        body->activate();
                    }
                }
            }
        }

        if (characterControlEnabled && !cutsceneActive && thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle && bowlObject && bowlQte) {
            float distToBowl = glm::distance(characterObject->position, bowlObject->position);
            const float activateDist = 2.0f;
            if (distToBowl < activateDist && !memoryTriggered) {
                if (!bowlQte->isActive()) bowlQte->setActive(true);
                glm::mat4 view = camera.GetViewMatrix();
                glm::mat4 proj = glm::perspective(glm::radians(camera.zoom), (float)screenWidth / screenHeight, 0.1f, 100.0f);
                bowlQte->setScreenSize(screenWidth, screenHeight);
                bowlQte->updateProjectedPosition(view, proj, bowlObject->position,
                                                 (float)screenWidth, (float)screenHeight,
                                                 0.8f, 150.0f, 15.0f);
            } else {
                if (bowlQte->isActive()) bowlQte->setActive(false);
            }
            if (bowlQte->isActive()) {
                bowlQte->update(deltaTime, window, GLFW_KEY_E);
                if (bowlQte->isHoldComplete()) {
                    std::cout << "[Memory] Started! Fading out." << std::endl;
                    memoryTriggered = true;
                    memoryActive = true;
                    memoryState = MemoryCinematicState::FadeOut;
                    memoryStartTime = currentFrame;
                    memorySavedExposure = camera.exposure;
                    memorySavedZoom = camera.zoom;

                    memorySavedCameraPos = camera.position;
                    memorySavedCameraFront = camera.front;
                    memorySavedCameraUp = camera.up;
                    memorySavedCameraRight = camera.right;
                    memorySavedYaw = camera.yaw;
                    memorySavedPitch = camera.pitch;

                    characterControlEnabled = false;
                    thirdPersonCameraActive = false;
                    thirdPersonTransitioning = false;
                }
            }
        }

        if (characterControlEnabled && !cutsceneActive && thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle && paperObject && paperQte && !paperMemoryTriggered) {
            float distToPaper = glm::distance(characterObject->position, paperObject->position);
            const float activateDist = 2.0f;
            if (distToPaper < activateDist) {
                if (!paperQte->isActive()) paperQte->setActive(true);
                glm::mat4 view = camera.GetViewMatrix();
                glm::mat4 proj = glm::perspective(glm::radians(camera.zoom), (float)screenWidth / screenHeight, 0.1f, 100.0f);
                paperQte->setScreenSize(screenWidth, screenHeight);
                paperQte->updateProjectedPosition(view, proj, paperObject->position,
                                                   (float)screenWidth, (float)screenHeight,
                                                   0.8f, 150.0f, 15.0f);
            } else {
                if (paperQte->isActive()) paperQte->setActive(false);
            }
            if (paperQte->isActive()) {
                paperQte->update(deltaTime, window, GLFW_KEY_E);
                if (paperQte->isHoldComplete()) {
                    std::cout << "[Paper Memory] Started!" << std::endl;
                    paperMemoryTriggered = true;
                    
                    paperSavedCameraPos = camera.position;
                    paperSavedCameraFront = camera.front;
                    paperSavedCameraUp = camera.up;
                    paperSavedCameraRight = camera.right;
                    paperSavedYaw = camera.yaw;
                    paperSavedPitch = camera.pitch;
                    paperSavedZoom = camera.zoom;
                    
                    characterControlEnabled = false;
                    thirdPersonCameraActive = false;
                    thirdPersonTransitioning = false;
                    
                    if (!paperMemoryAnimation) {
                        paperMemoryAnimation = ModelLoader::loadAnimation("../content/animations/act_1/Act_1_Paper_Memory.fbx");
                        if (paperMemoryAnimation) {
                            paperMemoryAnimation->setLooping(false);
                            paperMemoryAnimation->setAnimationSpeed(1.0f);
                            std::cout << "[Paper Memory] Animation loaded." << std::endl;
                        } else {
                            std::cerr << "[Paper Memory] Failed to load animation!" << std::endl;
                            paperMemoryState = PaperMemoryState::Idle;
                            characterControlEnabled = true;
                            thirdPersonCameraActive = true;
                            paperMemoryTriggered = false;
                            break;
                        }
                    }
                    
                    paperMemoryState = PaperMemoryState::Teleporting;
                    paperMemoryStartTime = currentFrame;
                }
            }
        }

        if (paperMemoryState != PaperMemoryState::Idle) {
            float elapsed = currentFrame - paperMemoryStartTime;
            
            switch (paperMemoryState) {
                case PaperMemoryState::Teleporting: {
                    camera.zoom = 55.0f;
                    
                    paperMemoryAnimation->play();
                    float duration = paperMemoryAnimation->getDuration() / paperMemoryAnimation->getAnimationSpeed();
                    paperCameraShake.start(glm::vec3(0.008f, 0.004f, 0.008f),
                                          glm::vec3(0.002f, 0.003f, 0.001f), 1.5f, duration);
                    
                    paperTextFading = true;
                    paperTextFadeStartTime = currentFrame;
                    paperTextAlpha = 0.0f;
                    
                    paperMemoryState = PaperMemoryState::Playing;
                    paperMemoryStartTime = currentFrame;
                    std::cout << "[Paper Memory] Teleported, playing animation and fading in text" << std::endl;
                    break;
                }
                
                case PaperMemoryState::Playing: {
                    paperMemoryAnimation->update(deltaTime);
                    
                    if (paperTextFading) {
                        float fadeElapsed = currentFrame - paperTextFadeStartTime;
                        paperTextAlpha = glm::clamp(fadeElapsed / 0.5f, 0.0f, 1.0f);
                        if (paperTextAlpha >= 1.0f) {
                            paperTextFading = false;
                            paperTextAlpha = 1.0f;
                        }
                    }
                    
                    float animTime = paperMemoryAnimation->getCurrentTime();
                    const AnimationChannel* camChannel = paperMemoryAnimation->getChannel("Camera");
                    if (camChannel) {
                        glm::vec3 pos = cleanCameraPosition;
                        glm::quat rot(1,0,0,0);
                        if (!camChannel->positionKeys.empty()) {
                            const KeyFrame* k = &camChannel->positionKeys[0];
                            float minD = std::abs(animTime - k->time);
                            for (const auto& key : camChannel->positionKeys) {
                                float d = std::abs(animTime - key.time);
                                if (d < minD) { minD = d; k = &key; }
                            }
                            pos = k->value.position;
                        }
                        if (!camChannel->rotationKeys.empty()) {
                            const KeyFrame* k = &camChannel->rotationKeys[0];
                            float minD = std::abs(animTime - k->time);
                            for (const auto& key : camChannel->rotationKeys) {
                                float d = std::abs(animTime - key.time);
                                if (d < minD) { minD = d; k = &key; }
                            }
                            rot = k->value.rotation;
                        }
                        cleanCameraPosition = pos;
                        glm::mat4 rotMatrix = glm::mat4_cast(rot);
                        cleanCameraFront = glm::normalize(glm::vec3(rotMatrix * glm::vec4(1,0,0,0)));
                        cleanCameraUp    = glm::normalize(glm::vec3(rotMatrix * glm::vec4(0,1,0,0)));
                        cleanCameraRight = glm::normalize(glm::cross(cleanCameraFront, cleanCameraUp));
                    }
                    
                    if (paperCameraShake.isActive()) {
                        ShakeOffset shake = paperCameraShake.update(deltaTime);
                        camera.position = cleanCameraPosition + shake.positionOffset;
                        if (glm::length(shake.rotationOffset) > 0.0001f) {
                            glm::quat q = glm::angleAxis(shake.rotationOffset.x, cleanCameraRight) *
                                         glm::angleAxis(shake.rotationOffset.y, cleanCameraUp) *
                                         glm::angleAxis(shake.rotationOffset.z, cleanCameraFront);
                            glm::mat3 rm = glm::mat3_cast(q);
                            camera.front = glm::normalize(rm * cleanCameraFront);
                            camera.up    = glm::normalize(rm * cleanCameraUp);
                            camera.right = glm::normalize(glm::cross(camera.front, camera.up));
                        } else {
                            camera.front = cleanCameraFront;
                            camera.up    = cleanCameraUp;
                            camera.right = cleanCameraRight;
                        }
                    } else {
                        camera.position = cleanCameraPosition;
                        camera.front = cleanCameraFront;
                        camera.up    = cleanCameraUp;
                        camera.right = cleanCameraRight;
                    }
                    
                    if (!paperMemoryAnimation->isPlaying()) {
                        std::cout << "[Paper Memory] Animation finished, returning to game" << std::endl;
                        paperMemoryState = PaperMemoryState::Returning;
                        paperMemoryStartTime = currentFrame;
                    }
                    break;
                }
                
                case PaperMemoryState::Returning: {
                    camera.zoom = paperSavedZoom;
                    camera.yaw = paperSavedYaw;
                    camera.pitch = paperSavedPitch;
                    camera.position = paperSavedCameraPos;
                    camera.front = paperSavedCameraFront;
                    camera.up = paperSavedCameraUp;
                    camera.right = paperSavedCameraRight;
                    
                    thirdPersonCameraActive = true;
                    thirdPersonTransitioning = false;
                    characterControlEnabled = true;
                    currentCharacterSpeed = 0.0f;
                    
                    paperTextAlpha = 0.0f;
                    
                    hope++;
                    std::cout << "[Hope] Increased to " << hope << std::endl;
                    
                    paperMemoryState = PaperMemoryState::Idle;
                    break;
                }
                
                default: break;
            }
        }

        if (memoryActive) {
            float elapsed = currentFrame - memoryStartTime;
            switch (memoryState) {
                case MemoryCinematicState::FadeOut: {
                    float t = glm::clamp(elapsed / memoryFadeOutDuration, 0.0f, 1.0f);
                    fadeOverlay->setAlpha(t);

                    if (characterObject && characterObject->isVisible() && characterBlender) {
                        characterBlender->clear();
                        characterBlender->setSource(idleAnimation, 1.0f);
                        characterBlender->updateAnimations(deltaTime);
                        characterBlender->applyToSkeleton(characterModel->getSkeleton(),
                                                         characterObject->getFinalModelMatrix(),
                                                         characterObject->scale);
                    }

                    if (t >= 1.0f) {
                        fadeOverlay->setAlpha(1.0f);
                        memoryState = MemoryCinematicState::BlackScreen;
                        memoryStartTime = currentFrame;
                        std::cout << "[Memory] Black screen started (0.3s) - teleporting camera and setting up scene" << std::endl;
                    }
                    break;
                }
                
                case MemoryCinematicState::BlackScreen: {
                    camera.zoom = 55.0f/2.0f;
                    
                    if (characterObject) characterObject->setVisible(false);
                    
                    if (memoryCharacterObject) {
                        memoryCharacterObject->setVisible(true);
                        memoryCharacterObject->position = glm::vec3(-4.21081f, 0.0f, 3.58422f);
                        if (memoryCharacterAnimManager && memoryCharacterIdleAnimation) {
                            memoryCharacterIdleAnimation->play();
                            memoryCharacterAnimManager->setAnimation(memoryCharacterIdleAnimation, memoryCharacterModel);
                        }
                    }
                    if (dogObject) {
                        dogObject->setVisible(true);
                        dogObject->position = glm::vec3(-4.44153f, 0.0f, 5.40058f);
                        if (dogAnimation) {
                            dogAnimation->play();
                        }
                    }
                    
                    if (!memoryAnimation) {
                        memoryAnimation = ModelLoader::loadAnimation("../content/animations/act_1/Act_1_Bowl_Memory.fbx");
                        if (memoryAnimation) {
                            memoryAnimation->setLooping(false);
                            memoryAnimation->setAnimationSpeed(1.0f);
                        } else {
                            std::cerr << "[Memory] Failed to load animation!" << std::endl;
                            memoryActive = false;
                            memoryState = MemoryCinematicState::Idle;
                            break;
                        }
                    }
                    memoryAnimation->play();
                    
                    if (dogPantingBuffer && !dogPantingSource && audioManager) {
                        dogPantingSource = audioManager->createSource(
                            dogPantingBuffer,
                            dogObject ? dogObject->position : glm::vec3(-4.44153f, 0.0f, 5.40058f),
                            1.5f, true, 1.0f, 15.0f, 1.0f
                        );
                    }
                    if (dogPantingSource && audioManager) {
                        audioManager->playSource(dogPantingSource);
                        std::cout << "[Memory] Dog panting sound started." << std::endl;
                    }
                    
                    float duration = memoryAnimation->getDuration() / memoryAnimation->getAnimationSpeed();
                    memoryCameraShake.start(glm::vec3(0.008f, 0.004f, 0.008f),
                                           glm::vec3(0.002f, 0.003f, 0.001f), 1.5f, duration);
                    
                    if (elapsed >= memoryBlackScreenDuration) {
                        std::cout << "[Memory] Black screen ended - starting fade in" << std::endl;
                        memoryState = MemoryCinematicState::Playing;
                        memoryStartTime = currentFrame;
                    }
                    break;
                }
                
                case MemoryCinematicState::Playing: {
                    if (elapsed < memoryFadeInDuration) {
                        float t = glm::clamp(elapsed / memoryFadeInDuration, 0.0f, 1.0f);
                        fadeOverlay->setAlpha(1.0f - t);
                    } else {
                        fadeOverlay->setAlpha(0.0f);
                    }

                    if (dogAnimManager && dogObject && dogObject->isVisible()) {
                        dogAnimManager->update(deltaTime);
                    }
                    
                    if (memoryCharacterAnimManager && memoryCharacterObject && memoryCharacterObject->isVisible()) {
                        memoryCharacterAnimManager->update(deltaTime);
                    }

                    memoryAnimation->update(deltaTime);
                    
                    if (!memoryDogPointLightActive && memoryAnimation) {
                        float currentFrameForLight = memoryAnimation->getCurrentTime() * 24.0f;
                        if (currentFrameForLight >= 121.0f) {
                            Light* dogPointLight = interiorScene->getLightByName("Dog Point Light");
                            if (!dogPointLight) {
                                dogPointLight = interiorScene->addPointLight("Dog Point Light");
                                dogPointLight->position = glm::vec3(-4.25687f, 0.894274f, 4.37401f);
                                dogPointLight->intensity = 0.05f;
                                dogPointLight->color = glm::vec3(1.0f, 0.95f, 0.8f);
                                dogPointLight->radius = 5.0f;
                                dogPointLight->setCastShadows(false);
                            }
                            dogPointLight->setActive(true);
                            memoryDogPointLightActive = true;
                            std::cout << "[Memory] Dog Point Light activated at frame " << currentFrameForLight << std::endl;
                        }
                    }
                    
                    float animTime = memoryAnimation->getCurrentTime();
                    const AnimationChannel* camChannel = memoryAnimation->getChannel("Camera");
                    if (camChannel) {
                        glm::vec3 pos = cleanCameraPosition;
                        glm::quat rot(1,0,0,0);
                        if (!camChannel->positionKeys.empty()) {
                            const KeyFrame* k = &camChannel->positionKeys[0];
                            float minD = std::abs(animTime - k->time);
                            for (const auto& key : camChannel->positionKeys) {
                                float d = std::abs(animTime - key.time);
                                if (d < minD) { minD = d; k = &key; }
                            }
                            pos = k->value.position;
                        }
                        if (!camChannel->rotationKeys.empty()) {
                            const KeyFrame* k = &camChannel->rotationKeys[0];
                            float minD = std::abs(animTime - k->time);
                            for (const auto& key : camChannel->rotationKeys) {
                                float d = std::abs(animTime - key.time);
                                if (d < minD) { minD = d; k = &key; }
                            }
                            rot = k->value.rotation;
                        }
                        cleanCameraPosition = pos;
                        glm::mat4 rotMatrix = glm::mat4_cast(rot);
                        cleanCameraFront = glm::normalize(glm::vec3(rotMatrix * glm::vec4(1,0,0,0)));
                        cleanCameraUp    = glm::normalize(glm::vec3(rotMatrix * glm::vec4(0,1,0,0)));
                        cleanCameraRight = glm::normalize(glm::cross(cleanCameraFront, cleanCameraUp));
                    }
                    if (memoryCameraShake.isActive()) {
                        ShakeOffset shake = memoryCameraShake.update(deltaTime);
                        camera.position = cleanCameraPosition + shake.positionOffset;
                        if (glm::length(shake.rotationOffset) > 0.0001f) {
                            glm::quat q = glm::angleAxis(shake.rotationOffset.x, cleanCameraRight) *
                                         glm::angleAxis(shake.rotationOffset.y, cleanCameraUp) *
                                         glm::angleAxis(shake.rotationOffset.z, cleanCameraFront);
                            glm::mat3 rm = glm::mat3_cast(q);
                            camera.front = glm::normalize(rm * cleanCameraFront);
                            camera.up    = glm::normalize(rm * cleanCameraUp);
                            camera.right = glm::normalize(glm::cross(camera.front, camera.up));
                        } else {
                            camera.front = cleanCameraFront;
                            camera.up    = cleanCameraUp;
                            camera.right = cleanCameraRight;
                        }
                    } else {
                        camera.position = cleanCameraPosition;
                        camera.front = cleanCameraFront;
                        camera.up    = cleanCameraUp;
                        camera.right = cleanCameraRight;
                    }
                    
                    if (dogPantingSource && dogObject && audioManager) {
                        audioManager->setSourcePosition(dogPantingSource, dogObject->position);
                    }
                    
                    if (!memoryAnimation->isPlaying()) {
                        if (memoryDogPointLightActive) {
                            Light* dogPointLight = interiorScene->getLightByName("Dog Point Light");
                            if (dogPointLight) {
                                dogPointLight->setActive(false);
                            }
                            memoryDogPointLightActive = false;
                        }
                        memoryStartTime = currentFrame;
                        memoryState = MemoryCinematicState::FadeOutPost;
                    }
                    break;
                }
                
                case MemoryCinematicState::FadeOutPost: {
                    float t = glm::clamp(elapsed / memoryFadeOutPostDuration, 0.0f, 1.0f);
                    fadeOverlay->setAlpha(t);
                    if (t >= 1.0f) {
                        fadeOverlay->setAlpha(1.0f);
                        memoryState = MemoryCinematicState::BlackScreenPost;
                        memoryStartTime = currentFrame;
                        std::cout << "[Memory] Black screen post started (0.3s) - restoring camera" << std::endl;
                    }
                    break;
                }
                
                case MemoryCinematicState::BlackScreenPost: {
                    camera.zoom = memorySavedZoom;

                    if (memoryCharacterObject) memoryCharacterObject->setVisible(false);
                    if (dogObject) dogObject->setVisible(false);
                    
                    if (dogPantingSource && audioManager) {
                        audioManager->stopSource(dogPantingSource);
                        audioManager->deleteSource(dogPantingSource);
                        dogPantingSource = 0;
                        std::cout << "[Memory] Dog panting sound stopped." << std::endl;
                    }
                    
                    if (elapsed >= memoryBlackScreenDuration) {
                        std::cout << "[Memory] Black screen post ended - fading in to game" << std::endl;
                        memoryState = MemoryCinematicState::FadeInPost;
                        memoryStartTime = currentFrame;
                    }
                    break;
                }
                
                case MemoryCinematicState::FadeInPost: {
                    float t = glm::clamp(elapsed / memoryFadeInPostDuration, 0.0f, 1.0f);
                    fadeOverlay->setAlpha(1.0f - t);
                    if (t >= 1.0f) {
                        fadeOverlay->setAlpha(0.0f);

                        thirdPersonCameraActive = true;
                        thirdPersonTransitioning = false;
                        camera.yaw = memorySavedYaw;
                        camera.pitch = memorySavedPitch;
                        camera.position = memorySavedCameraPos;
                        camera.front = memorySavedCameraFront;
                        camera.up = memorySavedCameraUp;
                        camera.right = memorySavedCameraRight;

                        if (characterObject) characterObject->setVisible(true);
                        characterControlEnabled = true;
                        currentCharacterSpeed = 0.0f;
                        hope++;
                        std::cout << "[Hope] Increased to " << hope << std::endl;

                        memoryActive = false;
                        memoryState = MemoryCinematicState::Idle;
                    }
                    break;
                }
                
                default: break;
            }
        }

        if (physicsWorld) {
            physicsWorld->stepSimulation(deltaTime);
            if (!cutsceneActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
                physicsWorld->syncDynamicObjects();
                if (characterJustTeleported) {
                    if (characterModel && characterModel->getMesh(0) && characterModel->getMesh(0)->collision) {
                        btRigidBody* body = characterModel->getMesh(0)->collision->getBody();
                        if (body) {
                            glm::vec3 targetPos = characterObject->position;
                            glm::vec3 offsetPos = characterModel->getMesh(0)->collision->getOffsetPosition();
                            btTransform trans;
                            trans.setIdentity();
                            trans.setOrigin(btVector3(targetPos.x + offsetPos.x, targetPos.y + offsetPos.y, targetPos.z + offsetPos.z));
                            btQuaternion rot;
                            rot.setRotation(btVector3(0, 1, 0), glm::radians(characterObject->rotation.y));
                            trans.setRotation(rot);
                            body->setWorldTransform(trans);
                            body->setLinearVelocity(btVector3(0, 0, 0));
                            body->setAngularVelocity(btVector3(0, 0, 0));
                            body->clearForces();
                            body->activate(true);
                            characterObject->position = targetPos;
                        }
                    }
                    characterJustTeleported = false;
                }
            }
        }

        if (thirdPersonCameraActive && !memoryActive && paperMemoryState == PaperMemoryState::Idle) {
            glm::vec3 target = characterObject->position + glm::vec3(0.0f, eyeHeight, 0.0f);
            float yawRad = glm::radians(camera.yaw);
            float pitchRad = glm::radians(camera.pitch);
            glm::vec3 front;
            front.x = cos(yawRad) * cos(pitchRad);
            front.y = sin(pitchRad);
            front.z = sin(yawRad) * cos(pitchRad);
            camera.front = glm::normalize(front);
            camera.right = glm::normalize(glm::cross(camera.front, glm::vec3(0.0f, 1.0f, 0.0f)));
            camera.up    = glm::normalize(glm::cross(camera.right, camera.front));
            camera.position = target - camera.front * thirdPersonDistance + camera.right * thirdPersonRightOffset;

            if (physicsWorld) {
                btVector3 btFrom(target.x, target.y, target.z);
                btVector3 btTo(camera.position.x, camera.position.y, camera.position.z);
                btCollisionWorld::ClosestRayResultCallback rayCallback(btFrom, btTo);
                rayCallback.m_collisionFilterMask = btBroadphaseProxy::StaticFilter;
                physicsWorld->getWorld()->rayTest(btFrom, btTo, rayCallback);
                if (rayCallback.hasHit()) {
                    const btVector3& hit = rayCallback.m_hitPointWorld;
                    glm::vec3 hitPos(hit.x(), hit.y(), hit.z());
                    glm::vec3 toTarget = glm::normalize(target - hitPos);
                    camera.position = hitPos + toTarget * 0.15f;
                }
            }
        }

        lightManager->update(deltaTime);

        if (audioManager) {
            audioManager->setListenerPosition(camera.position, camera.front, camera.up);
            
            if (!ambientPlaying && ambientSource) {
                audioManager->playSource(ambientSource);
                ambientPlaying = true;
                std::cout << "[Audio] Ambient background started." << std::endl;
            }
        }

        if (characterObject && characterObject->isVisible()) {
            currentScene->addExternalObject(characterObject);
        }
        renderer->renderShadowMaps(*currentScene);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 projection = glm::perspective(glm::radians(camera.zoom), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
        postProcessor->process(*currentScene, camera, screenWidth, screenHeight, currentScene->getPostSettings());
        if (characterObject && characterObject->isVisible()) {
            currentScene->removeExternalObject(characterObject);
        }

        if (qte && qte->isActive() && keyObject) {
            glm::vec3 keyWorldPos = keyObject->position;
            qte->setScreenSize(screenWidth, screenHeight);
            qte->updateProjectedPosition(view, projection, keyWorldPos,
                                         (float)screenWidth, (float)screenHeight,
                                         0.8f, 150.0f, 15.0f);
            qte->render();
        }

        if (bowlQte && bowlQte->isActive() && bowlObject) {
            bowlQte->render();
        }

        if (paperQte && paperQte->isActive() && paperObject) {
            paperQte->render();
        }

        if (paperText && paperTextAlpha > 0.0f) {
            paperText->setAlpha(paperTextAlpha);
            paperText->setScreenSize(screenWidth, screenHeight);
            int textWidth = paperText->getTextWidth();
            int textHeight = paperText->getTextHeight();
            paperText->setPosition((screenWidth - textWidth) / 2, screenHeight / 2 - textHeight / 2);
            paperText->render();
        }

        if (fadeOverlay) {
            fadeOverlay->render();
        }

        updateFPS(window);
        glfwSwapBuffers(window);
    }

    return 0;
}

void Engine::handleFootsteps(float deltaTime) {
    if (footstepBuffers.empty() || !audioManager) return;
    if (!footstepSourceLeft || !footstepSourceRight) return;
    if (!walkAnimation) return;

    float walkTime = walkAnimation->getCurrentTime();
    float duration = walkAnimation->getDuration();
    if (duration <= 0.0f) return;

    float cycleTime = fmod(walkTime, duration);

    float normalizedCycle = cycleTime / duration;

    const float rightStepNorm = 12.0f / 30.0f;       const float leftStepNorm  = 28.5f / 30.0f;   
    if (normalizedCycle < lastCycleTime) {
        footstepPhase = -1;       }
    lastCycleTime = normalizedCycle;

    if (normalizedCycle >= rightStepNorm && footstepPhase == -1) {
        footstepPhase = 0;           int idx = rand() % footstepBuffers.size();
        alSourceStop(footstepSourceRight);
        alSourcei(footstepSourceRight, AL_BUFFER, footstepBuffers[idx]);
        alSourcePlay(footstepSourceRight);
        std::cout << "[Footstep] RIGHT (frame 12), buffer " << idx << " at norm " << normalizedCycle << std::endl;
    }

    if (normalizedCycle >= leftStepNorm && footstepPhase == 0) {
        footstepPhase = 1;           int idx = rand() % footstepBuffers.size();
        alSourceStop(footstepSourceLeft);
        alSourcei(footstepSourceLeft, AL_BUFFER, footstepBuffers[idx]);
        alSourcePlay(footstepSourceLeft);
        std::cout << "[Footstep] LEFT (frame 29), buffer " << idx << " at norm " << normalizedCycle << std::endl;
    }
}


void Engine::framebufferSizeCallbackWrapper(GLFWwindow* window, int w, int h) {
    Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine) {
        engine->onWindowResize(w, h);
        if (engine->inputManager) engine->inputManager->framebufferSizeCallback(window, w, h);
    }
}

void Engine::mouseCallbackWrapper(GLFWwindow* window, double xpos, double ypos) {
    Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine && engine->inputManager &&
        (engine->freeCamera || engine->thirdPersonCameraActive || !engine->cutsceneActive))
        engine->inputManager->mouseCallback(window, xpos, ypos);
}

void Engine::scrollCallbackWrapper(GLFWwindow* window, double xoffset, double yoffset) {
    Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine) {
        if (engine->inputManager && !engine->thirdPersonCameraActive && (engine->freeCamera || !engine->cutsceneActive)) {
            engine->inputManager->scrollCallback(window, xoffset, yoffset);
        }
    }
}

void Engine::keyCallbackWrapper(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Engine* engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (engine) engine->keyCallback(window, key, scancode, action, mods);
}


void Engine::onWindowResize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    glViewport(0, 0, w, h);
}


void Engine::keyCallback(GLFWwindow* window, int key, int ,
                         int action, int ) {
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_F11: {
                GLFWmonitor* currentMonitor = glfwGetWindowMonitor(window);
                if (currentMonitor == nullptr) {
                    glfwGetWindowPos(window, &windowedPosX, &windowedPosY);
                    glfwGetWindowSize(window, &windowedWidth, &windowedHeight);
                    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                    glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
                    fullscreen = true;
                } else {
                    glfwSetWindowMonitor(window, nullptr, windowedPosX, windowedPosY, windowedWidth, windowedHeight, 0);
                    fullscreen = false;
                }
                break;
            }
        }
    }
}


void Engine::updateFPS(GLFWwindow* window) {
    frameCount++;
    float currentTime = glfwGetTime();
    float timeSinceLastUpdate = currentTime - lastFpsUpdate;
    if (timeSinceLastUpdate >= 1.0f) {
        fps = frameCount / timeSinceLastUpdate;
        std::stringstream title;
        title << "Rewind | FPS: " << std::fixed << std::setprecision(1) << fps;
        glfwSetWindowTitle(window, title.str().c_str());
        frameCount = 0;
        lastFpsUpdate = currentTime;
    }
}