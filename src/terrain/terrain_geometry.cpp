#include "terrain_internal.h"

#include <cmath>

namespace terrain_internal
{
// ----------------------------
// IsSideExposed
//
// Verifica se uma lateral precisa ser desenhada por estar acima do vizinho.
//
// ----------------------------
bool IsSideExposed(
    const TerrainPlane &terrain,
    int x,
    int z,
    float currentHeight)
{
    return GetCellHeight(terrain, x, z) < currentHeight - heightEpsilon;
}

// ----------------------------
// IsTopVisibleFromCamera
//
// Decide se o topo da celula esta acima da camera e pode aparecer.
//
// ----------------------------
bool IsTopVisibleFromCamera(
    const Camera3D &camera,
    float topY)
{
    return camera.position.y >= topY - heightEpsilon;
}

// ----------------------------
// GetTopColor
//
// Escolhe a cor do topo entre grama, leito de lago e plano.
//
// ----------------------------
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

// ----------------------------
// GetCellCorners
//
// Calcula os quatro cantos de uma celula no mundo.
//
// ----------------------------
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

namespace
{
// ----------------------------
// HeightsMatch
//
// Compara alturas com tolerancia para detectar blocos no mesmo nivel.
//
// ----------------------------
bool HeightsMatch(
    float first,
    float second)
{
    return std::fabs(first - second) <= 0.04f;
}

// ----------------------------
// ConsiderHalfCubeConnector
//
// Avalia se dois vizinhos formam um conector triangular valido.
//
// ----------------------------
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
}

// ----------------------------
// GetHalfCubeConnector
//
// Procura o melhor meio-cubo triangular para ligar blocos proximos.
//
// ----------------------------
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

} // namespace terrain_internal
