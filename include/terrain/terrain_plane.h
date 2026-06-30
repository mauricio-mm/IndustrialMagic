#ifndef TERRAIN_PLANE_H
#define TERRAIN_PLANE_H

#include "raylib.h"

#include <vector>

struct LakeWaterShader;

enum class TerrainMapKind
{
    FlatGrid,
    LowPolyWorld
};

struct TerrainPlane
{
    Vector3 origin;
    int widthCells;
    int heightCells;
    float cellSize;
    float maximumHeight;
    float lakeDepth;
    float waterLevel;
    float noiseScale;
    unsigned int seed;
    Color color;
    bool created;
    bool showMesh;
    TerrainMapKind mapKind;
    std::vector<float> cellHeights;
};

TerrainPlane CreateTerrainPlane();
void GenerateTerrainPlane(TerrainPlane *terrain);
void AdjustTerrainNoiseScale(
    TerrainPlane *terrain,
    float wheelMove);
void SetTerrainMapKind(
    TerrainPlane *terrain,
    TerrainMapKind mapKind);
const char *GetTerrainMapName(const TerrainPlane &terrain);
void DrawTerrainPlane(
    const TerrainPlane &terrain,
    const Camera3D &camera,
    const LakeWaterShader *lakeWater);

#endif
