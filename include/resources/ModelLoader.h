#ifndef MODELLOADER_H
#define MODELLOADER_H

#include "resources/Model.h"
#include "resources/Mesh.h"
#include "animation/Animation.h"
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <glm/glm.hpp>
#include <assimp/material.h>

struct aiMaterial;
struct aiScene;

class ModelLoader {
public:
    static Model* loadFBX(const std::string& filepath,
                         const std::string& textureBasePath = "");

    static Model* loadFBX(const std::string& filepath,
                         const std::string& albedoPath,
                         const std::string& normalPath,
                         const std::string& specularPath,
                         const std::string& aoPath,
                         const std::string& roughnessPath,
                         const std::string& metallicPath,
                         const std::string& alphaPath);
    
    static Mesh loadOBJ(const std::string& filepath, 
                        const std::string& albedoPath = "",
                        const std::string& normalPath = "",
                        const std::string& specularPath = "",
                        const std::string& aoPath = "",
                        const std::string& roughnessPath = "",
                        const std::string& metallicPath = "",
                        const std::string& alphaPath = "");
    
    static unsigned int loadTextureFromFile(const std::string& filename, bool gammaCorrection = false,
                                           const std::string& textureBasePath = "");
    static unsigned int createDefaultTexture();
    static Material loadPBRMaterial(const std::map<std::string, std::string>& texturePaths);
    static Animation* loadAnimation(const std::string& filepath);
    
    static std::vector<Bone> loadSkeleton(const std::string& filepath);
    
    static void printBoneAnimationDebug(Animation* animation, float currentTime, 
                                    const std::vector<std::string>& boneNames = {});

    static void clearTextureCache();

private:
    static std::unordered_map<std::string, unsigned int> textureCache;
    
    static unsigned int loadTextureFromMaterial(aiMaterial* mat, aiTextureType type,
                                                const aiScene* scene, bool gammaCorrection,
                                                const std::string& textureBasePath = "");
    
    static Model* loadFBXInternal(const std::string& filepath,
                                  const std::map<std::string, std::string>& texturePaths,
                                  bool loadFromFBXMaterials,
                                  const std::string& textureBasePath = "");

    static void processVertex(const std::vector<glm::vec3>& positions,
                              const std::vector<glm::vec3>& normals,
                              const std::vector<glm::vec2>& texCoords,
                              const std::string& vertex,
                              std::vector<Vertex>& vertices,
                              std::vector<unsigned int>& indices);
};

#endif