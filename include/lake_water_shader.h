#ifndef LAKE_WATER_SHADER_H
#define LAKE_WATER_SHADER_H

#include "raylib.h"

struct LakeWaterShader
{
    Shader shader;
    int timeLocation;
    int shallowColorLocation;
    int deepColorLocation;
    int highlightColorLocation;
    int edgeMaskLocation;
    bool ready;
};

LakeWaterShader CreateLakeWaterShader();
void UpdateLakeWaterShader(
    LakeWaterShader *water,
    float time);
void BeginLakeWaterShader(
    const LakeWaterShader *water,
    Vector4 edgeMask);
void EndLakeWaterShader(const LakeWaterShader *water);
void ShutdownLakeWaterShader(LakeWaterShader *water);

#endif
