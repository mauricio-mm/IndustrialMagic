#include "terrain_plane.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

namespace
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

constexpr Color topColor = { 77, 155, 79, 255 };
constexpr Color sideColor = { 124, 82, 47, 255 };
constexpr Color lakeBedColor = { 92, 118, 82, 255 };
constexpr Color flatTopColor = { 126, 176, 117, 255 };
constexpr Color meshColor = { 42, 50, 44, 220 };
constexpr Color edgeColor = { 72, 84, 74, 255 };
constexpr Color waterColor = { 48, 126, 184, 190 };

std::size_t GetCellIndex(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    return static_cast<std::size_t>(
        z * terrain.widthCells + x);
}

std::uint32_t HashCell(
    int x,
    int z,
    unsigned int seed)
{
    std::uint32_t value =
        static_cast<std::uint32_t>(x) * 374761393u +
        static_cast<std::uint32_t>(z) * 668265263u +
        seed * 2246822519u;
    value = (value ^ (value >> 13u)) * 1274126177u;
    return value ^ (value >> 16u);
}

float HashToUnitFloat(
    int x,
    int z,
    unsigned int seed)
{
    constexpr float divisor = 16777215.0f;
    return static_cast<float>(
        HashCell(x, z, seed) & 0x00FFFFFFu) / divisor;
}

float SmoothStep(float value)
{
    return value * value * (3.0f - 2.0f * value);
}

float Lerp(float start, float end, float amount)
{
    return start + (end - start) * amount;
}

float ValueNoise(
    float x,
    float z,
    float frequency,
    unsigned int seed)
{
    const float sampleX = x * frequency;
    const float sampleZ = z * frequency;
    const int x0 = static_cast<int>(std::floor(sampleX));
    const int z0 = static_cast<int>(std::floor(sampleZ));
    const int x1 = x0 + 1;
    const int z1 = z0 + 1;
    const float tx = SmoothStep(sampleX - static_cast<float>(x0));
    const float tz = SmoothStep(sampleZ - static_cast<float>(z0));

    const float top = Lerp(
        HashToUnitFloat(x0, z0, seed),
        HashToUnitFloat(x1, z0, seed),
        tx);
    const float bottom = Lerp(
        HashToUnitFloat(x0, z1, seed),
        HashToUnitFloat(x1, z1, seed),
        tx);
    return Lerp(top, bottom, tz);
}

float MaskAbove(
    float value,
    float threshold,
    float softness)
{
    return SmoothStep(std::clamp(
        (value - threshold) / softness,
        0.0f,
        1.0f));
}

float GetTerraceStep(const TerrainPlane &terrain)
{
    return std::max(minimumTerraceStep, terrain.maximumHeight / 28.0f);
}

float GetTerrainHeight(
    float x,
    float z,
    const TerrainPlane &terrain)
{
    if (terrain.mapKind == TerrainMapKind::FlatGrid)
    {
        return 0.0f;
    }

    const float scaledX = x / terrain.noiseScale;
    const float scaledZ = z / terrain.noiseScale;
    const float base =
        ValueNoise(scaledX, scaledZ, 0.045f, terrain.seed) * 0.56f +
        ValueNoise(
            scaledX + 17.0f,
            scaledZ - 11.0f,
            0.11f,
            terrain.seed + 41u) *
            0.28f +
        ValueNoise(
            scaledX - 5.0f,
            scaledZ + 23.0f,
            0.22f,
            terrain.seed + 79u) *
            0.16f;
    const float mountainMask = MaskAbove(
        ValueNoise(
            scaledX + 31.0f,
            scaledZ - 19.0f,
            0.032f,
            terrain.seed + 131u),
        0.72f,
        0.24f);
    const float basinMask = MaskAbove(
        ValueNoise(
            scaledX - 29.0f,
            scaledZ + 13.0f,
            0.043f,
            terrain.seed + 197u),
        0.69f,
        0.25f);
    const float plainMask = MaskAbove(
        ValueNoise(
            scaledX + 7.0f,
            scaledZ + 37.0f,
            0.055f,
            terrain.seed + 251u),
        0.61f,
        0.32f);

    float height = terrain.maximumHeight * (0.08f + base * 0.20f);
    height +=
        mountainMask * terrain.maximumHeight * (0.34f + base * 0.42f);
    height -= basinMask * terrain.lakeDepth * (0.72f + base * 0.28f);
    height = Lerp(
        height,
        terrain.maximumHeight * (0.08f + base * 0.035f),
        plainMask * (1.0f - mountainMask) * (1.0f - basinMask) * 0.75f);

    height = std::clamp(height, -terrain.lakeDepth, terrain.maximumHeight);

    const float terraceStep = GetTerraceStep(terrain);
    return std::round(height / terraceStep) * terraceStep;
}

float GetTerrainBottom(const TerrainPlane &terrain)
{
    return terrain.origin.y - terrain.lakeDepth - 1.0f;
}

float GetCellHeight(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    if (x < 0 || x >= terrain.widthCells ||
        z < 0 || z >= terrain.heightCells)
    {
        return -terrain.lakeDepth - 1.0f;
    }

    return terrain.cellHeights[GetCellIndex(terrain, x, z)];
}

float GetCellBottomY(
    const TerrainPlane &terrain,
    float bottomY,
    int x,
    int z)
{
    return std::max(
        bottomY,
        terrain.origin.y + GetCellHeight(terrain, x, z));
}

bool IsSideExposed(
    const TerrainPlane &terrain,
    int x,
    int z,
    float currentHeight)
{
    return GetCellHeight(terrain, x, z) < currentHeight - heightEpsilon;
}

bool IsTopVisibleFromCamera(
    const Camera3D &camera,
    float topY)
{
    return camera.position.y >= topY - heightEpsilon;
}

Vector3 GetSideNormal(TerrainSide side)
{
    switch (side)
    {
        case TerrainSide::North:
            return { 0.0f, 0.0f, -1.0f };
        case TerrainSide::East:
            return { 1.0f, 0.0f, 0.0f };
        case TerrainSide::South:
            return { 0.0f, 0.0f, 1.0f };
        case TerrainSide::West:
            return { -1.0f, 0.0f, 0.0f };
    }

    return { 0.0f, 0.0f, 0.0f };
}

bool IsSideFacingCamera(
    const Camera3D &camera,
    TerrainSide side,
    float minX,
    float maxX,
    float minZ,
    float maxZ)
{
    Vector3 faceCenter = {};
    switch (side)
    {
        case TerrainSide::North:
            faceCenter = {
                (minX + maxX) * 0.5f,
                0.0f,
                minZ
            };
            break;
        case TerrainSide::East:
            faceCenter = {
                maxX,
                0.0f,
                (minZ + maxZ) * 0.5f
            };
            break;
        case TerrainSide::South:
            faceCenter = {
                (minX + maxX) * 0.5f,
                0.0f,
                maxZ
            };
            break;
        case TerrainSide::West:
            faceCenter = {
                minX,
                0.0f,
                (minZ + maxZ) * 0.5f
            };
            break;
    }

    const Vector3 normal = GetSideNormal(side);
    const Vector3 toCamera = {
        camera.position.x - faceCenter.x,
        0.0f,
        camera.position.z - faceCenter.z
    };

    return normal.x * toCamera.x + normal.z * toCamera.z > -0.001f;
}

Color GetTopColor(
    const TerrainPlane &terrain,
    float height)
{
    if (terrain.mapKind == TerrainMapKind::FlatGrid)
    {
        return flatTopColor;
    }

    if (height <= terrain.waterLevel + 0.03f)
    {
        return lakeBedColor;
    }

    return topColor;
}

Vector3 Raise(Vector3 position, float amount)
{
    position.y += amount;
    return position;
}

void DrawQuad(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Vector3 d,
    Color color)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, d, color);
}

void DrawDoubleSidedTriangle(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Color color)
{
    DrawTriangle3D(a, b, c, color);
    DrawTriangle3D(a, c, b, color);
}

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
    Vector3 *southWest)
{
    const float minX = startX + static_cast<float>(x) * cellSize;
    const float maxX = minX + cellSize;
    const float minZ = startZ + static_cast<float>(z) * cellSize;
    const float maxZ = minZ + cellSize;

    *northWest = { minX, northWestY, minZ };
    *northEast = { maxX, northEastY, minZ };
    *southEast = { maxX, southEastY, maxZ };
    *southWest = { minX, southWestY, maxZ };
}

bool HeightsMatch(
    float first,
    float second)
{
    return std::fabs(first - second) <= 0.04f;
}

void ConsiderHalfCubeConnector(
    const TerrainPlane &terrain,
    float currentHeight,
    float firstHeight,
    float secondHeight,
    HalfCubeCorner corner,
    HalfCubeConnector *bestConnector)
{
    const float minimumRise = GetTerraceStep(terrain) * 0.6f;
    if (!HeightsMatch(firstHeight, secondHeight) ||
        firstHeight <= currentHeight + minimumRise)
    {
        return;
    }

    if (bestConnector->corner == HalfCubeCorner::None ||
        firstHeight - currentHeight >
            bestConnector->height - currentHeight)
    {
        bestConnector->corner = corner;
        bestConnector->height = firstHeight;
    }
}

HalfCubeConnector GetHalfCubeConnector(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    HalfCubeConnector connector = {
        HalfCubeCorner::None,
        0.0f
    };

    if (terrain.mapKind == TerrainMapKind::FlatGrid)
    {
        return connector;
    }

    const float currentHeight = GetCellHeight(terrain, x, z);
    ConsiderHalfCubeConnector(
        terrain,
        currentHeight,
        GetCellHeight(terrain, x - 1, z),
        GetCellHeight(terrain, x, z - 1),
        HalfCubeCorner::NorthWest,
        &connector);
    ConsiderHalfCubeConnector(
        terrain,
        currentHeight,
        GetCellHeight(terrain, x + 1, z),
        GetCellHeight(terrain, x, z - 1),
        HalfCubeCorner::NorthEast,
        &connector);
    ConsiderHalfCubeConnector(
        terrain,
        currentHeight,
        GetCellHeight(terrain, x + 1, z),
        GetCellHeight(terrain, x, z + 1),
        HalfCubeCorner::SouthEast,
        &connector);
    ConsiderHalfCubeConnector(
        terrain,
        currentHeight,
        GetCellHeight(terrain, x - 1, z),
        GetCellHeight(terrain, x, z + 1),
        HalfCubeCorner::SouthWest,
        &connector);

    return connector;
}

void DrawTopMesh(
    Vector3 northWest,
    Vector3 northEast,
    Vector3 southEast,
    Vector3 southWest)
{
    northWest = Raise(northWest, meshOffset);
    northEast = Raise(northEast, meshOffset);
    southEast = Raise(southEast, meshOffset);
    southWest = Raise(southWest, meshOffset);

    DrawLine3D(northWest, northEast, meshColor);
    DrawLine3D(northEast, southEast, meshColor);
    DrawLine3D(southEast, southWest, meshColor);
    DrawLine3D(southWest, northWest, meshColor);
}

void DrawMeshEdge(
    Vector3 start,
    Vector3 end)
{
    DrawLine3D(
        Raise(start, meshOffset),
        Raise(end, meshOffset),
        meshColor);
}

void DrawTriangleMesh(
    Vector3 a,
    Vector3 b,
    Vector3 c)
{
    DrawMeshEdge(a, b);
    DrawMeshEdge(b, c);
    DrawMeshEdge(c, a);
}

void DrawVerticalFaceMesh(
    Vector3 highA,
    Vector3 highB,
    Vector3 lowB,
    Vector3 lowA)
{
    DrawMeshEdge(highA, highB);
    DrawMeshEdge(highB, lowB);
    DrawMeshEdge(lowB, lowA);
    DrawMeshEdge(lowA, highA);
}

void DrawSideMesh(
    TerrainSide side,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    float topY,
    float bottomY)
{
    switch (side)
    {
        case TerrainSide::North:
            DrawVerticalFaceMesh(
                { minX, topY, minZ },
                { maxX, topY, minZ },
                { maxX, bottomY, minZ },
                { minX, bottomY, minZ });
            break;
        case TerrainSide::East:
            DrawVerticalFaceMesh(
                { maxX, topY, minZ },
                { maxX, topY, maxZ },
                { maxX, bottomY, maxZ },
                { maxX, bottomY, minZ });
            break;
        case TerrainSide::South:
            DrawVerticalFaceMesh(
                { maxX, topY, maxZ },
                { minX, topY, maxZ },
                { minX, bottomY, maxZ },
                { maxX, bottomY, maxZ });
            break;
        case TerrainSide::West:
            DrawVerticalFaceMesh(
                { minX, topY, maxZ },
                { minX, topY, minZ },
                { minX, bottomY, minZ },
                { minX, bottomY, maxZ });
            break;
    }
}

void DrawOneSidedVerticalFace(
    Vector3 highA,
    Vector3 highB,
    Vector3 lowB,
    Vector3 lowA,
    Color color)
{
    DrawTriangle3D(highA, highB, lowB, color);
    DrawTriangle3D(highA, lowB, lowA, color);
}

void DrawVerticalFace(
    Vector3 highA,
    Vector3 highB,
    Vector3 lowB,
    Vector3 lowA,
    Color color)
{
    DrawOneSidedVerticalFace(highA, highB, lowB, lowA, color);
    DrawOneSidedVerticalFace(highB, highA, lowA, lowB, color);
}

void DrawSideFace(
    TerrainSide side,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    float topY,
    float bottomY,
    Color color)
{
    switch (side)
    {
        case TerrainSide::North:
            DrawOneSidedVerticalFace(
                { minX, topY, minZ },
                { maxX, topY, minZ },
                { maxX, bottomY, minZ },
                { minX, bottomY, minZ },
                color);
            break;
        case TerrainSide::East:
            DrawOneSidedVerticalFace(
                { maxX, topY, minZ },
                { maxX, topY, maxZ },
                { maxX, bottomY, maxZ },
                { maxX, bottomY, minZ },
                color);
            break;
        case TerrainSide::South:
            DrawOneSidedVerticalFace(
                { maxX, topY, maxZ },
                { minX, topY, maxZ },
                { minX, bottomY, maxZ },
                { maxX, bottomY, maxZ },
                color);
            break;
        case TerrainSide::West:
            DrawOneSidedVerticalFace(
                { minX, topY, maxZ },
                { minX, topY, minZ },
                { minX, bottomY, minZ },
                { minX, bottomY, maxZ },
                color);
            break;
    }
}

void DrawExposedSide(
    const Camera3D &camera,
    TerrainSide side,
    float minX,
    float maxX,
    float minZ,
    float maxZ,
    float topY,
    float bottomY,
    bool showMesh)
{
    if (bottomY >= topY - heightEpsilon ||
        !IsSideFacingCamera(camera, side, minX, maxX, minZ, maxZ))
    {
        return;
    }

    DrawSideFace(
        side,
        minX,
        maxX,
        minZ,
        maxZ,
        topY,
        bottomY,
        sideColor);

    if (showMesh)
    {
        DrawSideMesh(side, minX, maxX, minZ, maxZ, topY, bottomY);
    }
}

void DrawHalfCubeConnector(
    HalfCubeConnector connector,
    Vector3 northWest,
    Vector3 northEast,
    Vector3 southEast,
    Vector3 southWest,
    float highY,
    bool showMesh)
{
    if (connector.corner == HalfCubeCorner::None)
    {
        return;
    }

    Vector3 highNorthWest = { northWest.x, highY, northWest.z };
    Vector3 highNorthEast = { northEast.x, highY, northEast.z };
    Vector3 highSouthEast = { southEast.x, highY, southEast.z };
    Vector3 highSouthWest = { southWest.x, highY, southWest.z };

    switch (connector.corner)
    {
        case HalfCubeCorner::NorthWest:
            DrawDoubleSidedTriangle(
                highNorthWest,
                highNorthEast,
                highSouthWest,
                topColor);
            DrawVerticalFace(
                highNorthEast,
                highSouthWest,
                southWest,
                northEast,
                sideColor);
            if (showMesh)
            {
                DrawTriangleMesh(
                    highNorthWest,
                    highNorthEast,
                    highSouthWest);
                DrawVerticalFaceMesh(
                    highNorthEast,
                    highSouthWest,
                    southWest,
                    northEast);
            }
            break;
        case HalfCubeCorner::NorthEast:
            DrawDoubleSidedTriangle(
                highNorthEast,
                highSouthEast,
                highNorthWest,
                topColor);
            DrawVerticalFace(
                highNorthWest,
                highSouthEast,
                southEast,
                northWest,
                sideColor);
            if (showMesh)
            {
                DrawTriangleMesh(
                    highNorthEast,
                    highSouthEast,
                    highNorthWest);
                DrawVerticalFaceMesh(
                    highNorthWest,
                    highSouthEast,
                    southEast,
                    northWest);
            }
            break;
        case HalfCubeCorner::SouthEast:
            DrawDoubleSidedTriangle(
                highSouthEast,
                highSouthWest,
                highNorthEast,
                topColor);
            DrawVerticalFace(
                highNorthEast,
                highSouthWest,
                southWest,
                northEast,
                sideColor);
            if (showMesh)
            {
                DrawTriangleMesh(
                    highSouthEast,
                    highSouthWest,
                    highNorthEast);
                DrawVerticalFaceMesh(
                    highNorthEast,
                    highSouthWest,
                    southWest,
                    northEast);
            }
            break;
        case HalfCubeCorner::SouthWest:
            DrawDoubleSidedTriangle(
                highSouthWest,
                highNorthWest,
                highSouthEast,
                topColor);
            DrawVerticalFace(
                highNorthWest,
                highSouthEast,
                southEast,
                northWest,
                sideColor);
            if (showMesh)
            {
                DrawTriangleMesh(
                    highSouthWest,
                    highNorthWest,
                    highSouthEast);
                DrawVerticalFaceMesh(
                    highNorthWest,
                    highSouthEast,
                    southEast,
                    northWest);
            }
            break;
        case HalfCubeCorner::None:
            break;
    }
}

void DrawWaterTile(
    float startX,
    float startZ,
    float cellSize,
    int x,
    int z,
    float waterY)
{
    DrawPlane(
        {
            startX + (static_cast<float>(x) + 0.5f) * cellSize,
            waterY + waterOffset,
            startZ + (static_cast<float>(z) + 0.5f) * cellSize
        },
        { cellSize, cellSize },
        waterColor);
}
}

TerrainPlane CreateTerrainPlane()
{
    TerrainPlane terrain = {};
    terrain.origin = { 0.0f, 0.0f, 0.0f };
    terrain.widthCells = 32;
    terrain.heightCells = 32;
    terrain.cellSize = 1.0f;
    terrain.maximumHeight = 15.0f;
    terrain.lakeDepth = 4.0f;
    terrain.waterLevel = 0.0f;
    terrain.noiseScale = 1.0f;
    terrain.seed = 1337u;
    terrain.color = { 220, 224, 218, 255 };
    terrain.created = true;
    terrain.showMesh = false;
    terrain.mapKind = TerrainMapKind::LowPolyWorld;
    GenerateTerrainPlane(&terrain);
    return terrain;
}

void GenerateTerrainPlane(TerrainPlane *terrain)
{
    if (terrain->widthCells <= 0 || terrain->heightCells <= 0)
    {
        terrain->cellHeights.clear();
        terrain->created = false;
        return;
    }

    terrain->cellHeights.assign(
        static_cast<std::size_t>(
            terrain->widthCells * terrain->heightCells),
        0.0f);

    const float width =
        static_cast<float>(terrain->widthCells) * terrain->cellSize;
    const float height =
        static_cast<float>(terrain->heightCells) * terrain->cellSize;
    const float startX = terrain->origin.x - width * 0.5f;
    const float startZ = terrain->origin.z - height * 0.5f;

    for (int z = 0; z < terrain->heightCells; ++z)
    {
        for (int x = 0; x < terrain->widthCells; ++x)
        {
            const float worldX =
                startX +
                (static_cast<float>(x) + 0.5f) * terrain->cellSize;
            const float worldZ =
                startZ +
                (static_cast<float>(z) + 0.5f) * terrain->cellSize;

            terrain->cellHeights[GetCellIndex(*terrain, x, z)] =
                GetTerrainHeight(worldX, worldZ, *terrain);
        }
    }

    terrain->created = true;
}

void AdjustTerrainNoiseScale(
    TerrainPlane *terrain,
    float wheelMove)
{
    if (wheelMove == 0.0f)
    {
        return;
    }

    terrain->noiseScale = std::clamp(
        terrain->noiseScale + wheelMove * noiseScaleStep,
        minimumNoiseScale,
        maximumNoiseScale);
    GenerateTerrainPlane(terrain);
}

void SetTerrainMapKind(
    TerrainPlane *terrain,
    TerrainMapKind mapKind)
{
    if (terrain->mapKind == mapKind)
    {
        return;
    }

    terrain->mapKind = mapKind;
    terrain->showMesh = mapKind == TerrainMapKind::FlatGrid;
    GenerateTerrainPlane(terrain);
}

const char *GetTerrainMapName(const TerrainPlane &terrain)
{
    switch (terrain.mapKind)
    {
        case TerrainMapKind::FlatGrid:
            return "Plano com malha";
        case TerrainMapKind::LowPolyWorld:
            return "Mundo quadrado";
    }

    return "Terreno";
}

void DrawTerrainPlane(
    const TerrainPlane &terrain,
    const Camera3D &camera)
{
    if (!terrain.created || terrain.cellHeights.empty())
    {
        return;
    }

    const float width =
        static_cast<float>(terrain.widthCells) * terrain.cellSize;
    const float height =
        static_cast<float>(terrain.heightCells) * terrain.cellSize;
    const float startX = terrain.origin.x - width * 0.5f;
    const float startZ = terrain.origin.z - height * 0.5f;
    const float bottomY = GetTerrainBottom(terrain);

    for (int z = 0; z < terrain.heightCells; ++z)
    {
        for (int x = 0; x < terrain.widthCells; ++x)
        {
            const float currentHeight = GetCellHeight(terrain, x, z);
            const float lowTopY = terrain.origin.y + currentHeight;
            const float minX =
                startX + static_cast<float>(x) * terrain.cellSize;
            const float maxX = minX + terrain.cellSize;
            const float minZ =
                startZ + static_cast<float>(z) * terrain.cellSize;
            const float maxZ = minZ + terrain.cellSize;

            Vector3 northWest = {};
            Vector3 northEast = {};
            Vector3 southEast = {};
            Vector3 southWest = {};
            GetCellCorners(
                startX,
                startZ,
                terrain.cellSize,
                x,
                z,
                lowTopY + topOffset,
                lowTopY + topOffset,
                lowTopY + topOffset,
                lowTopY + topOffset,
                &northWest,
                &northEast,
                &southEast,
                &southWest);

            if (IsTopVisibleFromCamera(camera, lowTopY))
            {
                DrawQuad(
                    northWest,
                    southWest,
                    southEast,
                    northEast,
                    GetTopColor(terrain, currentHeight));

                if (terrain.showMesh)
                {
                    DrawTopMesh(northWest, northEast, southEast, southWest);
                }
            }

            if (IsSideExposed(terrain, x, z - 1, currentHeight))
            {
                DrawExposedSide(
                    camera,
                    TerrainSide::North,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + topOffset,
                    GetCellBottomY(terrain, bottomY, x, z - 1),
                    terrain.showMesh);
            }

            if (IsSideExposed(terrain, x + 1, z, currentHeight))
            {
                DrawExposedSide(
                    camera,
                    TerrainSide::East,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + topOffset,
                    GetCellBottomY(terrain, bottomY, x + 1, z),
                    terrain.showMesh);
            }

            if (IsSideExposed(terrain, x, z + 1, currentHeight))
            {
                DrawExposedSide(
                    camera,
                    TerrainSide::South,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + topOffset,
                    GetCellBottomY(terrain, bottomY, x, z + 1),
                    terrain.showMesh);
            }

            if (IsSideExposed(terrain, x - 1, z, currentHeight))
            {
                DrawExposedSide(
                    camera,
                    TerrainSide::West,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + topOffset,
                    GetCellBottomY(terrain, bottomY, x - 1, z),
                    terrain.showMesh);
            }

            const HalfCubeConnector connector =
                GetHalfCubeConnector(terrain, x, z);
            if (connector.corner != HalfCubeCorner::None)
            {
                DrawHalfCubeConnector(
                    connector,
                    northWest,
                    northEast,
                    southEast,
                    southWest,
                    terrain.origin.y + connector.height + topOffset,
                    terrain.showMesh);
            }

            if (terrain.mapKind == TerrainMapKind::LowPolyWorld &&
                lowTopY < terrain.origin.y + terrain.waterLevel)
            {
                DrawWaterTile(
                    startX,
                    startZ,
                    terrain.cellSize,
                    x,
                    z,
                    terrain.origin.y + terrain.waterLevel);
            }
        }
    }

    DrawLine3D(
        { startX, terrain.origin.y + meshOffset, startZ },
        { startX + width, terrain.origin.y + meshOffset, startZ },
        edgeColor);
    DrawLine3D(
        { startX + width, terrain.origin.y + meshOffset, startZ },
        { startX + width, terrain.origin.y + meshOffset, startZ + height },
        edgeColor);
    DrawLine3D(
        { startX + width, terrain.origin.y + meshOffset, startZ + height },
        { startX, terrain.origin.y + meshOffset, startZ + height },
        edgeColor);
    DrawLine3D(
        { startX, terrain.origin.y + meshOffset, startZ + height },
        { startX, terrain.origin.y + meshOffset, startZ },
        edgeColor);
}
