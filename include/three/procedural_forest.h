#ifndef PROCEDURAL_FOREST_H
#define PROCEDURAL_FOREST_H

#include "raylib.h"
#include "terrain/terrain_plane.h"

#include <vector>

struct ProceduralBranch
{
    Vector3 start;
    Vector3 end;
    float startRadius;
    float endRadius;
};

struct ProceduralLeaf
{
    Vector3 center;
    float radius;
    Color color;
};

struct ProceduralTree
{
    Vector3 trunkStart;
    Vector3 trunkEnd;
    float trunkRadius;
    std::vector<ProceduralBranch> branches;
    std::vector<ProceduralLeaf> leaves;
};

struct ProceduralForest
{
    int widthCells;
    int heightCells;
    float maximumHeight;
    float lakeDepth;
    float noiseScale;
    float waterLevel;
    unsigned int seed;
    TerrainMapKind mapKind;
    std::vector<ProceduralTree> trees;
};

ProceduralForest CreateProceduralForest();
void UpdateProceduralForest(
    ProceduralForest *forest,
    const TerrainPlane &terrain);
void DrawProceduralForest(const ProceduralForest &forest);

#endif
