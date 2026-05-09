#include "AnimatedModel.h"
#include <unordered_map>
#include <cmath>

// -----------------------------------------------------------------------
void VertexBoneData::AddBoneData(int boneID, float weight) {
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
        if (Weights[i] == 0.0f) {
            IDs[i]     = (float)boneID;
            Weights[i] = weight;
            return;
        }
    }
}

// -----------------------------------------------------------------------
AnimatedModel::AnimatedModel() {}

AnimatedModel::~AnimatedModel() {
    for (auto& m : meshes) {
        glDeleteVertexArrays(1, &m.VAO);
        glDeleteBuffers(1, &m.VBO);
        glDeleteBuffers(1, &m.IBO);
    }
    for (auto* t : textures) if (t) delete t;
}

// -----------------------------------------------------------------------
glm::mat4 AnimatedModel::ToGlm(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}
glm::vec3 AnimatedModel::ToGlm(const aiVector3D& v) { return { v.x, v.y, v.z }; }
glm::quat AnimatedModel::ToGlm(const aiQuaternion& q) { return { q.w, q.x, q.y, q.z }; }

// -----------------------------------------------------------------------
void AnimatedModel::LoadModel(const std::string& fileName) {
    scene = importer.ReadFile(fileName,
        aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_JoinIdenticalVertices);
    if (!scene) {
        printf("AnimatedModel: fallo al cargar %s: %s\n", fileName.c_str(), importer.GetErrorString());
        return;
    }
    globalInverse = glm::inverse(ToGlm(scene->mRootNode->mTransformation));
    LoadMeshes(scene);
    LoadMaterials(scene);
    LoadAnimations(scene);
    printf("AnimatedModel: %s cargado — %d huesos, %d animaciones\n",
           fileName.c_str(), numBones, (int)clips.size());
}

// -----------------------------------------------------------------------
void AnimatedModel::LoadMeshes(const aiScene* sc) {
    // Registrar todos los huesos primero (índice global único por nombre)
    for (unsigned m = 0; m < sc->mNumMeshes; m++) {
        aiMesh* mesh = sc->mMeshes[m];
        for (unsigned b = 0; b < mesh->mNumBones; b++) {
            std::string name(mesh->mBones[b]->mName.data);
            if (boneMap.find(name) == boneMap.end()) {
                BoneInfo bi;
                bi.OffsetMatrix = ToGlm(mesh->mBones[b]->mOffsetMatrix);
                boneMap[name]   = numBones;
                bones.push_back(bi);
                numBones++;
            }
        }
    }
    for (unsigned m = 0; m < sc->mNumMeshes; m++)
        LoadSkinnedMesh(sc->mMeshes[m]);
}

void AnimatedModel::LoadSkinnedMesh(aiMesh* mesh) {
    // Pesos por vértice
    std::vector<VertexBoneData> boneData(mesh->mNumVertices);
    for (unsigned b = 0; b < mesh->mNumBones; b++) {
        std::string name(mesh->mBones[b]->mName.data);
        int idx = boneMap[name];
        for (unsigned w = 0; w < mesh->mBones[b]->mNumWeights; w++) {
            boneData[mesh->mBones[b]->mWeights[w].mVertexId]
                    .AddBoneData(idx, mesh->mBones[b]->mWeights[w].mWeight);
        }
    }

    // Buffer intercalado: pos(3) uv(2) norm(3) boneIDs(4) boneWeights(4) = 16 floats
    std::vector<float> verts;
    verts.reserve(mesh->mNumVertices * 16);
    for (unsigned i = 0; i < mesh->mNumVertices; i++) {
        verts.push_back(mesh->mVertices[i].x);
        verts.push_back(mesh->mVertices[i].y);
        verts.push_back(mesh->mVertices[i].z);
        if (mesh->mTextureCoords[0]) {
            verts.push_back(mesh->mTextureCoords[0][i].x);
            verts.push_back(mesh->mTextureCoords[0][i].y);
        } else { verts.push_back(0); verts.push_back(0); }
        if (mesh->mNormals) {
            verts.push_back(-mesh->mNormals[i].x);
            verts.push_back(-mesh->mNormals[i].y);
            verts.push_back(-mesh->mNormals[i].z);
        } else { verts.push_back(0); verts.push_back(0); verts.push_back(-1); }
        for (int k = 0; k < MAX_BONE_INFLUENCE; k++) verts.push_back(boneData[i].IDs[k]);
        for (int k = 0; k < MAX_BONE_INFLUENCE; k++) verts.push_back(boneData[i].Weights[k]);
    }

    std::vector<unsigned int> indices;
    for (unsigned f = 0; f < mesh->mNumFaces; f++) {
        aiFace& face = mesh->mFaces[f];
        for (unsigned j = 0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }

    SkinnedEntry entry;
    entry.indexCount    = (unsigned int)indices.size();
    entry.materialIndex = mesh->mMaterialIndex;

    glGenVertexArrays(1, &entry.VAO);
    glGenBuffers(1, &entry.VBO);
    glGenBuffers(1, &entry.IBO);

    glBindVertexArray(entry.VAO);
    glBindBuffer(GL_ARRAY_BUFFER, entry.VBO);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, entry.IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    GLsizei stride = 16 * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);                      glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3  * sizeof(float)));   glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5  * sizeof(float)));   glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)(8  * sizeof(float)));   glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)(12 * sizeof(float)));   glEnableVertexAttribArray(4);
    glBindVertexArray(0);

    meshes.push_back(entry);
    meshToTex.push_back(mesh->mMaterialIndex);
}

// -----------------------------------------------------------------------
void AnimatedModel::LoadMaterials(const aiScene* sc) {
    std::unordered_map<std::string, Texture*> loaded;
    textures.resize(sc->mNumMaterials, nullptr);
    for (unsigned i = 0; i < sc->mNumMaterials; i++) {
        aiMaterial* mat = sc->mMaterials[i];
        aiTextureType tt = aiTextureType_NONE;
        if (mat->GetTextureCount(aiTextureType_DIFFUSE))    tt = aiTextureType_DIFFUSE;
        else if (mat->GetTextureCount(aiTextureType_BASE_COLOR)) tt = aiTextureType_BASE_COLOR;

        if (tt != aiTextureType_NONE) {
            aiString path;
            if (mat->GetTexture(tt, 0, &path) == AI_SUCCESS) {
                std::string ps(path.data);
                if (ps[0] == '*') {
                    int ei = atoi(ps.c_str() + 1);
                    if (ei < (int)sc->mNumTextures) {
                        aiTexture* et = sc->mTextures[ei];
                        Texture* t = new Texture();
                        bool ok = (et->mHeight == 0)
                            ? t->LoadTextureFromMemory((unsigned char*)et->pcData, et->mWidth)
                            : t->LoadTextureFromRawPixels((unsigned char*)et->pcData, et->mWidth, et->mHeight);
                        if (ok) textures[i] = t; else delete t;
                    }
                } else {
                    std::string fn = ps.substr(ps.find_last_of("/\\") + 1);
                    std::string tp = "Textures/" + fn;
                    auto it = loaded.find(tp);
                    if (it != loaded.end()) {
                        textures[i] = it->second;
                    } else {
                        Texture* t = new Texture(tp.c_str());
                        std::string ext = fn.substr(fn.find_last_of('.') + 1);
                        bool ok = (ext == "tga" || ext == "png") ? t->LoadTextureA() : t->LoadTexture();
                        if (ok) { textures[i] = t; loaded[tp] = t; } else delete t;
                    }
                }
            }
        }
        if (!textures[i]) {
            textures[i] = new Texture("Textures/plain.png");
            textures[i]->LoadTextureA();
        }
    }
}

// -----------------------------------------------------------------------
void AnimatedModel::LoadAnimations(const aiScene* sc) {
    for (unsigned a = 0; a < sc->mNumAnimations; a++) {
        aiAnimation* anim = sc->mAnimations[a];
        AnimClip clip;
        clip.name           = anim->mName.data;
        clip.duration       = (float)anim->mDuration;
        clip.ticksPerSecond = (float)(anim->mTicksPerSecond > 0 ? anim->mTicksPerSecond : 25.0);

        for (unsigned c = 0; c < anim->mNumChannels; c++) {
            aiNodeAnim* ch = anim->mChannels[c];
            BoneChannel bc;
            bc.boneName = ch->mNodeName.data;
            for (unsigned k = 0; k < ch->mNumPositionKeys; k++)
                bc.positions.push_back({ ToGlm(ch->mPositionKeys[k].mValue),  (float)ch->mPositionKeys[k].mTime });
            for (unsigned k = 0; k < ch->mNumRotationKeys; k++)
                bc.rotations.push_back({ ToGlm(ch->mRotationKeys[k].mValue),  (float)ch->mRotationKeys[k].mTime });
            for (unsigned k = 0; k < ch->mNumScalingKeys; k++)
                bc.scales.push_back(   { ToGlm(ch->mScalingKeys[k].mValue),   (float)ch->mScalingKeys[k].mTime });
            clip.channelMap[bc.boneName] = (int)clip.channels.size();
            clip.channels.push_back(bc);
        }
        clips.push_back(clip);
    }
}

// -----------------------------------------------------------------------
int AnimatedModel::FindAnimation(const std::string& name) const {
    for (int i = 0; i < (int)clips.size(); i++)
        if (clips[i].name == name) return i;
    return 0;
}
std::string AnimatedModel::GetAnimationName(int i) const {
    return (i >= 0 && i < (int)clips.size()) ? clips[i].name : "";
}

// -----------------------------------------------------------------------
// Interpolación
glm::vec3 AnimatedModel::LerpPos(float t, const BoneChannel& ch) {
    if (ch.positions.size() == 1) return ch.positions[0].value;
    for (int i = 0; i < (int)ch.positions.size() - 1; i++) {
        if (t < ch.positions[i+1].time) {
            float dt = ch.positions[i+1].time - ch.positions[i].time;
            float f  = (t - ch.positions[i].time) / dt;
            return ch.positions[i].value + f * (ch.positions[i+1].value - ch.positions[i].value);
        }
    }
    return ch.positions.back().value;
}

glm::quat AnimatedModel::NlerpRot(float t, const BoneChannel& ch) {
    if (ch.rotations.size() == 1) return ch.rotations[0].value;
    for (int i = 0; i < (int)ch.rotations.size() - 1; i++) {
        if (t < ch.rotations[i+1].time) {
            float dt = ch.rotations[i+1].time - ch.rotations[i].time;
            float f  = (t - ch.rotations[i].time) / dt;
            glm::quat q1 = ch.rotations[i].value;
            glm::quat q2 = ch.rotations[i+1].value;
            // Asegura el camino corto
            if (q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w < 0.0f) q2 = -q2;
            glm::quat r;
            r.x = q1.x + f*(q2.x - q1.x);
            r.y = q1.y + f*(q2.y - q1.y);
            r.z = q1.z + f*(q2.z - q1.z);
            r.w = q1.w + f*(q2.w - q1.w);
            return glm::normalize(r);
        }
    }
    return ch.rotations.back().value;
}

glm::vec3 AnimatedModel::LerpScale(float t, const BoneChannel& ch) {
    if (ch.scales.size() == 1) return ch.scales[0].value;
    for (int i = 0; i < (int)ch.scales.size() - 1; i++) {
        if (t < ch.scales[i+1].time) {
            float dt = ch.scales[i+1].time - ch.scales[i].time;
            float f  = (t - ch.scales[i].time) / dt;
            return ch.scales[i].value + f * (ch.scales[i+1].value - ch.scales[i].value);
        }
    }
    return ch.scales.back().value;
}

// -----------------------------------------------------------------------
void AnimatedModel::Update(float timeSec, const std::string& animName) {
    if (clips.empty() || !scene) return;
    int idx = FindAnimation(animName);
    const AnimClip& clip = clips[idx];

    float tps        = clip.ticksPerSecond > 0 ? clip.ticksPerSecond : 25.0f;
    float timeInTicks = fmodf(timeSec * tps, clip.duration);

    for (auto& b : bones) b.FinalTransform = glm::mat4(1.0f);
    TraverseNode(timeInTicks, clip, scene->mRootNode, glm::mat4(1.0f));
}

void AnimatedModel::TraverseNode(float tickTime, const AnimClip& clip,
                                  aiNode* node, const glm::mat4& parentTransform)
{
    std::string nodeName(node->mName.data);
    glm::mat4 nodeTransform = ToGlm(node->mTransformation);

    auto it = clip.channelMap.find(nodeName);
    if (it != clip.channelMap.end()) {
        const BoneChannel& ch = clip.channels[it->second];
        glm::mat4 T = glm::translate(glm::mat4(1.0f), LerpPos(tickTime, ch));
        glm::mat4 R = glm::mat4_cast(NlerpRot(tickTime, ch));
        glm::mat4 S = glm::scale(glm::mat4(1.0f), LerpScale(tickTime, ch));
        nodeTransform = T * R * S;
    }

    glm::mat4 globalTransform = parentTransform * nodeTransform;

    auto boneIt = boneMap.find(nodeName);
    if (boneIt != boneMap.end()) {
        int i = boneIt->second;
        bones[i].FinalTransform = globalInverse * globalTransform * bones[i].OffsetMatrix;
    }

    for (unsigned i = 0; i < node->mNumChildren; i++)
        TraverseNode(tickTime, clip, node->mChildren[i], globalTransform);
}

// -----------------------------------------------------------------------
void AnimatedModel::Render(GLuint bonesUniformLoc) {
    // Subir matrices de huesos al shader
    glm::mat4 boneMatrices[MAX_BONES];
    for (int i = 0; i < MAX_BONES; i++)
        boneMatrices[i] = (i < numBones) ? bones[i].FinalTransform : glm::mat4(1.0f);
    glUniformMatrix4fv(bonesUniformLoc, MAX_BONES, GL_FALSE, glm::value_ptr(boneMatrices[0]));

    for (unsigned i = 0; i < meshes.size(); i++) {
        unsigned ti = meshToTex[i];
        if (ti < textures.size() && textures[ti])
            textures[ti]->UseTexture();
        glBindVertexArray(meshes[i].VAO);
        glDrawElements(GL_TRIANGLES, meshes[i].indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }
}
