#ifndef PROCEDURAL_ROCKS_H
#define PROCEDURAL_ROCKS_H

#include "raylib.h"
#include "terrain/terrain_plane.h"

#include <vector>

struct ProceduralStone
{
    Vector3 center;
    Vector3 scale;
    float rotationY;
    Color color;
};

struct ProceduralRockCluster
{
    std::vector<ProceduralStone> stones;
};

struct ProceduralRocks
{
    int widthCells;
    int heightCells;
    float maximumHeight;
    float lakeDepth;
    float noiseScale;
    float waterLevel;
    unsigned int seed;
    TerrainMapKind mapKind;
    std::vector<ProceduralRockCluster> clusters;
};

ProceduralRocks CreateProceduralRocks();
void UpdateProceduralRocks(
    ProceduralRocks *rocks,
    const TerrainPlane &terrain);
void DrawProceduralRocks(
    const ProceduralRocks &rocks,
    bool showMesh);

#endif
