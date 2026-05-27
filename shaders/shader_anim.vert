#version 330

layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 tex;
layout (location = 2) in vec3 norm;
layout (location = 3) in vec4 boneIDs;
layout (location = 4) in vec4 boneWeights;

out vec4 vCol;
out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
out vec4 vColor;

uniform mat4 model;
uniform mat4 projection;
uniform mat4 view;
uniform vec3 color;
uniform mat4 gBones[66];

void main()
{
    mat4 bt  = gBones[int(boneIDs.x)] * boneWeights.x;
         bt += gBones[int(boneIDs.y)] * boneWeights.y;
         bt += gBones[int(boneIDs.z)] * boneWeights.z;
         bt += gBones[int(boneIDs.w)] * boneWeights.w;

    float totalW = boneWeights.x + boneWeights.y + boneWeights.z + boneWeights.w;
    if (totalW < 0.001) bt = mat4(1.0);

    vec4 posL  = bt * vec4(pos, 1.0);
    vec3 normL = (bt * vec4(norm, 0.0)).xyz;

    gl_Position = projection * view * model * posL;
    vCol        = vec4(0.0, 1.0, 0.0, 1.0);
    vColor      = vec4(color, 1.0);
    TexCoord    = tex;
    Normal      = mat3(transpose(inverse(model))) * normL;
    FragPos     = (model * posL).xyz;
}
