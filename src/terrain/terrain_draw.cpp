#include "terrain_internal.h"

#include "raymath.h"

#include <algorithm>

namespace terrain_internal
{
namespace
{
constexpr float ambientSunLight = 0.48f;
constexpr float directSunLight  = 0.58f;

// ----------------------------
// GetSunDirection
//
// Retorna a direcao usada para iluminar o terreno como luz do sol.
//
// ----------------------------
Vector3 GetSunDirection()
{
    return Vector3Normalize({ -0.52f, 0.82f, -0.24f });
}

// ----------------------------
// ScaleColorChannel
//
// Ajusta um canal de cor pela intensidade calculada da luz.
//
// ----------------------------
unsigned char ScaleColorChannel(
    unsigned char value,
    float intensity)
{
    const int scaledValue = static_cast<int>(
        static_cast<float>(value) * intensity);
    return static_cast<unsigned char>(std::clamp(scaledValue, 0, 255));
}

// ----------------------------
// ApplySunLighting
//
// Aplica luz ambiente e luz direcional do sol a uma cor base.
//
// ----------------------------
Color ApplySunLighting(
    Color color,
    Vector3 normal)
{
    const Vector3 surfaceNormal = Vector3Normalize(normal);
    const float sunAmount       = std::max(
        0.0f,
        Vector3DotProduct(surfaceNormal, GetSunDirection()));
    const float intensity       = ambientSunLight + sunAmount * directSunLight;

    return {
        ScaleColorChannel(color.r, intensity),
        ScaleColorChannel(color.g, intensity),
        ScaleColorChannel(color.b, intensity),
        color.a
    };
}

// ----------------------------
// ColorsMatch
//
// Compara duas cores incluindo transparencia.
//
// ----------------------------
bool ColorsMatch(
    Color first,
    Color second)
{
    return first.r == second.r &&
        first.g == second.g &&
        first.b == second.b &&
        first.a == second.a;
}

// ----------------------------
// IsGrassColor
//
// Identifica as cores de grama que ja carregam luz e sombra proprias.
//
// ----------------------------
bool IsGrassColor(Color color)
{
    return ColorsMatch(color, grassLightColor) ||
        ColorsMatch(color, grassShadowColor) ||
        ColorsMatch(color, flatTopColor);
}

// ----------------------------
// ApplySurfaceLighting
//
// Mantem a paleta da grama e aplica luz comum nas outras superficies.
//
// ----------------------------
Color ApplySurfaceLighting(
    Color color,
    Vector3 normal)
{
    if (IsGrassColor(color))
    {
        return color;
    }

    return ApplySunLighting(color, normal);
}

// ----------------------------
// GetFaceNormal
//
// Calcula a normal de uma face a partir de tres vertices.
//
// ----------------------------
Vector3 GetFaceNormal(
    Vector3 a,
    Vector3 b,
    Vector3 c)
{
    return Vector3Normalize(
        Vector3CrossProduct(
            Vector3Subtract(b, a),
            Vector3Subtract(c, a)));
}

// ----------------------------
// Raise
//
// Eleva um ponto um pouco para desenhar linhas sem brigar com a face.
//
// ----------------------------
Vector3 Raise(Vector3 position, float amount)
{
    position.y += amount;
    return position;
}

// ----------------------------
// DrawQuad
//
// Desenha um quadrado a partir de dois triangulos.
//
// ----------------------------
void DrawQuad(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Vector3 d,
    Color color)
{
    const Color litColor = ApplySurfaceLighting(
        color,
        { 0.0f, 1.0f, 0.0f });

    DrawTriangle3D(a, b, c, litColor);
    DrawTriangle3D(a, c, d, litColor);
}

// ----------------------------
// DrawDoubleSidedTriangle
//
// Desenha um triangulo dos dois lados para evitar desaparecimento pela camera.
//
// ----------------------------
void DrawDoubleSidedTriangle(
    Vector3 a,
    Vector3 b,
    Vector3 c,
    Color color)
{
    const Color litColor = ApplySurfaceLighting(
        color,
        { 0.0f, 1.0f, 0.0f });

    DrawTriangle3D(a, b, c, litColor);
    DrawTriangle3D(a, c, b, litColor);
}

// ----------------------------
// GetSideNormal
//
// Retorna a normal horizontal de uma lateral do bloco.
//
// ----------------------------
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

// ----------------------------
// IsSideFacingCamera
//
// Verifica se uma lateral esta virada para a camera.
//
// ----------------------------
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

// ----------------------------
// DrawMeshEdge
//
// Desenha uma aresta da malha com um pequeno deslocamento.
//
// ----------------------------
void DrawMeshEdge(
    Vector3 start,
    Vector3 end)
{
    DrawLine3D(
        Raise(start, meshOffset),
        Raise(end, meshOffset),
        meshColor);
}

// ----------------------------
// DrawTriangleMesh
//
// Desenha as tres arestas de uma face triangular.
//
// ----------------------------
void DrawTriangleMesh(
    Vector3 a,
    Vector3 b,
    Vector3 c)
{
    DrawMeshEdge(a, b);
    DrawMeshEdge(b, c);
    DrawMeshEdge(c, a);
}

// ----------------------------
// DrawVerticalFaceMesh
//
// Desenha o contorno de uma face vertical.
//
// ----------------------------
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

// ----------------------------
// DrawSideMesh
//
// Desenha a malha de uma lateral especifica do bloco.
//
// ----------------------------
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

// ----------------------------
// DrawOneSidedVerticalFace
//
// Desenha uma face vertical em apenas uma orientacao.
//
// ----------------------------
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

// ----------------------------
// DrawVerticalFace
//
// Desenha uma face vertical dos dois lados.
//
// ----------------------------
void DrawVerticalFace(
    Vector3 highA,
    Vector3 highB,
    Vector3 lowB,
    Vector3 lowA,
    Color color)
{
    const Color litColor = ApplySunLighting(
        color,
        GetFaceNormal(highA, highB, lowB));

    DrawOneSidedVerticalFace(highA, highB, lowB, lowA, litColor);
    DrawOneSidedVerticalFace(highB, highA, lowA, lowB, litColor);
}

// ----------------------------
// DrawSideFace
//
// Monta e desenha uma lateral marrom de acordo com sua direcao.
//
// ----------------------------
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
    const Color litColor = ApplySunLighting(color, GetSideNormal(side));

    switch (side)
    {
        case TerrainSide::North:
            DrawOneSidedVerticalFace(
                { minX, topY, minZ },
                { maxX, topY, minZ },
                { maxX, bottomY, minZ },
                { minX, bottomY, minZ },
                litColor);
            break;
        case TerrainSide::East:
            DrawOneSidedVerticalFace(
                { maxX, topY, minZ },
                { maxX, topY, maxZ },
                { maxX, bottomY, maxZ },
                { maxX, bottomY, minZ },
                litColor);
            break;
        case TerrainSide::South:
            DrawOneSidedVerticalFace(
                { maxX, topY, maxZ },
                { minX, topY, maxZ },
                { minX, bottomY, maxZ },
                { maxX, bottomY, maxZ },
                litColor);
            break;
        case TerrainSide::West:
            DrawOneSidedVerticalFace(
                { minX, topY, maxZ },
                { minX, topY, minZ },
                { minX, bottomY, minZ },
                { minX, bottomY, maxZ },
                litColor);
            break;
    }
}
}

// ----------------------------
// DrawTopTile
//
// Desenha o topo verde ou plano de uma celula.
//
// ----------------------------
void DrawTopTile(
    Vector3 northWest,
    Vector3 northEast,
    Vector3 southEast,
    Vector3 southWest,
    Color color)
{
    DrawQuad(northWest, southWest, southEast, northEast, color);
}

// ----------------------------
// DrawTopMesh
//
// Desenha o contorno superior de uma celula.
//
// ----------------------------
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

// ----------------------------
// DrawExposedSide
//
// Desenha uma lateral exposta quando ela esta visivel pela camera.
//
// ----------------------------
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

// ----------------------------
// DrawHalfCubeConnector
//
// Desenha o meio-cubo triangular entre dois blocos no mesmo nivel.
//
// ----------------------------
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

// ----------------------------
// DrawWaterTile
//
// Desenha uma placa de agua sobre celulas abaixo do nivel do lago.
//
// ----------------------------
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
        ApplySunLighting(waterColor, { 0.0f, 1.0f, 0.0f }));
}

} // namespace terrain_internal
