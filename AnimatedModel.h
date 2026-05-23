#pragma once
#include <vector>
#include <string>
#include <map>
#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>
#include <glew.h>
#include <assimp\Importer.hpp>
#include <assimp\scene.h>
#include <assimp\postprocess.h>
#include "Texture.h"

#define MAX_BONES         66
#define MAX_BONE_INFLUENCE 4

struct VertexBoneData {
    float IDs[MAX_BONE_INFLUENCE]     = {0,0,0,0};
    float Weights[MAX_BONE_INFLUENCE] = {0,0,0,0};
    void AddBoneData(int boneID, float weight);
};

struct BoneInfo {
    glm::mat4 OffsetMatrix   = glm::mat4(1.0f);
    glm::mat4 FinalTransform = glm::mat4(1.0f);
};

struct KeyPosition { glm::vec3 value; float time; };
struct KeyRotation { glm::quat value; float time; };
struct KeyScale    { glm::vec3 value; float time; };

struct BoneChannel {
    std::string boneName;
    std::vector<KeyPosition> positions;
    std::vector<KeyRotation> rotations;
    std::vector<KeyScale>    scales;
};

struct AnimClip {
    std::string name;
    float duration;
    float ticksPerSecond;
    std::vector<BoneChannel>    channels;
    std::map<std::string, int>  channelMap;
};

struct SkinnedEntry {
    GLuint VAO = 0, VBO = 0, IBO = 0;
    unsigned int indexCount    = 0;
    unsigned int materialIndex = 0;
};

class AnimatedModel {
public:
    AnimatedModel();
    ~AnimatedModel();

    void LoadModel(const std::string& fileName);
    void Update(float timeSec, const std::string& animName = "walk", bool inPlace = false);
    void Render(GLuint bonesUniformLoc);

    int         FindAnimation(const std::string& name) const;
    std::string GetAnimationName(int i) const;
    int         GetAnimationCount() const { return (int)clips.size(); }

private:
    std::vector<SkinnedEntry>    meshes;
    std::vector<Texture*>        textures;
    std::vector<unsigned int>    meshToTex;

    std::map<std::string, int>   boneMap;
    std::vector<BoneInfo>        bones;
    int                          numBones = 0;
    glm::mat4                    globalInverse;

    std::vector<AnimClip>        clips;

    Assimp::Importer importer;
    const aiScene*   scene = nullptr;

    void LoadMeshes(const aiScene* sc);
    void LoadSkinnedMesh(aiMesh* mesh);
    void LoadMaterials(const aiScene* sc);
    void LoadAnimations(const aiScene* sc);

    void TraverseNode(float tickTime, const AnimClip& clip,
                      aiNode* node, const glm::mat4& parent, bool inPlace);

    glm::vec3 LerpPos(float t, const BoneChannel& ch);
    glm::quat NlerpRot(float t, const BoneChannel& ch);
    glm::vec3 LerpScale(float t, const BoneChannel& ch);

    static glm::mat4 ToGlm(const aiMatrix4x4& m);
    static glm::vec3 ToGlm(const aiVector3D& v);
    static glm::quat ToGlm(const aiQuaternion& q);
};
