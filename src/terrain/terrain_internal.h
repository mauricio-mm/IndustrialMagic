#ifndef TERRAIN_INTERNAL_H
#define TERRAIN_INTERNAL_H

#include "raylib.h"
#include "terrain/terrain_plane.h"

#include <cstddef>

struct LakeWaterShader;

namespace terrain_internal
{
enum class HalfCubeCorner
{
    None,
    NorthWest,
    NorthEast,
    SouthEast,
    SouthWest
};

enum class TerrainSide
{
    North,
    East,
    South,
    West
};

struct HalfCubeConnector
{
    HalfCubeCorner corner;
    float height;
};

constexpr float meshOffset = 0.034f;
constexpr float topOffset = 0.014f;
constexpr float waterOffset = 0.018f;
constexpr float heightEpsilon = 0.04f;
constexpr float minimumTerraceStep = 0.5f;
constexpr float minimumNoiseScale = 0.45f;
constexpr float maximumNoiseScale = 3.5f;
constexpr float noiseScaleStep = 0.12f;

constexpr Color grassLightColor  = { 168, 190, 109, 255 };
constexpr Color grassShadowColor = { 99, 135, 118, 255 };
constexpr Color topColor         = { 168, 190, 109, 255 };
constexpr Color sideColor        = { 124, 82, 47, 255 };
constexpr Color lakeBedColor     = { 92, 118, 82, 255 };
constexpr Color flatTopColor     = { 168, 190, 109, 255 };
constexpr Color meshColor        = { 42, 50, 44, 220 };
constexpr Color edgeColor        = { 72, 84, 74, 255 };
constexpr Color waterColor       = { 48, 126, 184, 190 };

std::size_t GetCellIndex(
    const TerrainPlane &terrain,
    int x,
    int z);
float GetTerraceStep(const TerrainPlane &terrain);
float GetTerrainHeight(
    float x,
    float z,
    const TerrainPlane &terrain);
float GetTerrainBottom(const TerrainPlane &terrain);
float GetCellHeight(
    const TerrainPlane &terrain,
    int x,
    int z);
float GetCellBottomY(
    const TerrainPlane &terrain,
    float bottomY,
    int x,
    int z);
bool IsSideExposed(
    const TerrainPlane &terrain,
    int x,
    int z,
    float currentHeight);
bool IsTopVisibleFromCamera(
    const Camera3D &camera,
    float topY);
Color GetTopColor(
    const TerrainPlane &terrain,
    int x,
    int z,
    float height);
void GetCellCorners(
    float startX,
    float startZ,
    float cellSize,
    int x,
    int z,
    float northWestY,
    float northEastY,
    float southEastY,
    float southWestY,
    Vector3 *northWest,
    Vector3 *northEast,
    Vector3 *southEast,
    Vector3 *southWest);
HalfCubeConnector GetHalfCubeConnector(
    const TerrainPlane &terrain,
    int x,
    int z);
void DrawTopTile(
    Vector3 northWest,
    Vector3 northEast,
    Vector3 southEast,
    Vector3 southWest,
    Color color);
void DrawTopMesh(
    Vector3 northWest,
    Vector3 northEast,
    Vector3 southEast,
    Vector3 southWest);
void DrawExposedSide(
    const Camera3D &camera,
    TerrainSide side,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    float topY,
    float bottomY,
    bool showMesh);
void DrawHalfCubeConnector(
    HalfCubeConnector connector,
    Vector3 northWest,
    Vector3 northEast,
    Vector3 southEast,
    Vector3 southWest,
    float highY,
    bool showMesh);
void DrawWaterTile(
    float startX,
    float startZ,
    float cellSize,
    int x,
    int z,
    float waterY,
    Vector4 edgeMask,
    HalfCubeCorner landCorner,
    const LakeWaterShader *lakeWater);

} // namespace terrain_internal

#endif
