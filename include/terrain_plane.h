#ifndef TERRAIN_PLANE_H
#define TERRAIN_PLANE_H

#include "raylib.h"

struct TerrainPlane
{
    Vector3 origin;
    int widthCells;
    int heightCells;
    float cellSize;
    Color color;
    bool created;
};

TerrainPlane CreateTerrainPlane();
void DrawTerrainPlane(const TerrainPlane &terrain);

#endif
