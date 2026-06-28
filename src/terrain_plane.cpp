#include "terrain_plane.h"

namespace
{
constexpr float surfaceOffset = 0.01f;
constexpr float lineOffset = 0.012f;
constexpr Color lightCellColor = { 245, 247, 250, 255 };
constexpr Color darkCellColor = { 214, 219, 226, 255 };
constexpr Color edgeColor = { 92, 101, 112, 255 };
}

TerrainPlane CreateTerrainPlane()
{
    TerrainPlane terrain = {};
    terrain.origin = { 0.0f, 0.0f, 0.0f };
    terrain.widthCells = 8;
    terrain.heightCells = 8;
    terrain.cellSize = 1.0f;
    terrain.color = { 220, 224, 218, 255 };
    terrain.created = true;
    return terrain;
}

void DrawTerrainPlane(const TerrainPlane &terrain)
{
    if (!terrain.created)
    {
        return;
    }

    const float width =
        static_cast<float>(terrain.widthCells) * terrain.cellSize;
    const float height =
        static_cast<float>(terrain.heightCells) * terrain.cellSize;
    const float startX = terrain.origin.x - width * 0.5f;
    const float startZ = terrain.origin.z - height * 0.5f;
    const float surfaceY = terrain.origin.y - surfaceOffset;
    const float lineY = terrain.origin.y + lineOffset;

    for (int z = 0; z < terrain.heightCells; ++z)
    {
        for (int x = 0; x < terrain.widthCells; ++x)
        {
            const Vector3 center = {
                startX + (static_cast<float>(x) + 0.5f) *
                    terrain.cellSize,
                surfaceY,
                startZ + (static_cast<float>(z) + 0.5f) *
                    terrain.cellSize
            };
            const Color color =
                ((x + z) % 2 == 0) ? lightCellColor : darkCellColor;

            DrawPlane(
                center,
                { terrain.cellSize, terrain.cellSize },
                color);
        }
    }

    DrawLine3D(
        { startX, lineY, startZ },
        { startX + width, lineY, startZ },
        edgeColor);
    DrawLine3D(
        { startX + width, lineY, startZ },
        { startX + width, lineY, startZ + height },
        edgeColor);
    DrawLine3D(
        { startX + width, lineY, startZ + height },
        { startX, lineY, startZ + height },
        edgeColor);
    DrawLine3D(
        { startX, lineY, startZ + height },
        { startX, lineY, startZ },
        edgeColor);
}
