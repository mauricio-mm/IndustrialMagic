#include "three/procedural_rocks.h"

#include "rlgl.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
constexpr float groundOffset          = 0.045f;
constexpr float minimumRockClearance  = 0.08f;
constexpr int maximumRockClusterCount = 180;
constexpr int rockRings               = 4;
constexpr int rockSlices              = 6;

constexpr Color rockLightColor  = { 128, 127, 105, 255 };
constexpr Color rockMidColor    = { 93, 98, 86, 255 };
constexpr Color rockShadowColor = { 60, 69, 66, 255 };
constexpr Color rockMeshColor   = { 35, 41, 38, 220 };

struct RockCandidate
{
    int x;
    int z;
    float score;
};

// ----------------------------
// MixHash
//
// Mistura bits de um inteiro para gerar valores pseudoaleatorios estaveis.
//
// ----------------------------
std::uint32_t MixHash(std::uint32_t value)
{
    value ^= value >> 16;
    value *= 0x7feb352du;
    value ^= value >> 15;
    value *= 0x846ca68bu;
    value ^= value >> 16;
    return value;
}

// ----------------------------
// HashCell
//
// Gera um hash deterministico para uma celula e um salt.
//
// ----------------------------
std::uint32_t HashCell(
    unsigned int seed,
    int x,
    int z,
    int salt)
{
    const std::uint32_t baseSeed = static_cast<std::uint32_t>(seed);
    const std::uint32_t hashX    =
        static_cast<std::uint32_t>(x) * 0x9e3779b1u;
    const std::uint32_t hashZ    =
        static_cast<std::uint32_t>(z) * 0x85ebca77u;
    const std::uint32_t hashSalt =
        static_cast<std::uint32_t>(salt) * 0xc2b2ae3du;

    return MixHash(baseSeed ^ hashX ^ hashZ ^ hashSalt);
}

// ----------------------------
// Hash01
//
// Converte o hash da celula para um valor entre 0 e 1.
//
// ----------------------------
float Hash01(
    unsigned int seed,
    int x,
    int z,
    int salt)
{
    return static_cast<float>(HashCell(seed, x, z, salt) & 0x00ffffffu) /
        16777215.0f;
}

// ----------------------------
// RandomRange
//
// Mapeia um hash da celula para uma faixa numerica.
//
// ----------------------------
float RandomRange(
    unsigned int seed,
    int x,
    int z,
    int salt,
    float minimumValue,
    float maximumValue)
{
    return minimumValue +
        (maximumValue - minimumValue) * Hash01(seed, x, z, salt);
}

// ----------------------------
// GetCellIndex
//
// Converte coordenadas de celula para indice no vetor de alturas.
//
// ----------------------------
std::size_t GetCellIndex(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    return static_cast<std::size_t>(
        z * terrain.widthCells + x);
}

// ----------------------------
// IsInsideTerrain
//
// Verifica se a celula esta dentro dos limites do terreno.
//
// ----------------------------
bool IsInsideTerrain(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    return x >= 0 &&
        z >= 0 &&
        x < terrain.widthCells &&
        z < terrain.heightCells;
}

// ----------------------------
// GetCellHeight
//
// Retorna a altura de uma celula ou um valor baixo fora do mapa.
//
// ----------------------------
float GetCellHeight(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    if (!IsInsideTerrain(terrain, x, z))
    {
        return -10000.0f;
    }

    return terrain.cellHeights[GetCellIndex(terrain, x, z)];
}

// ----------------------------
// GetSlopeScore
//
// Mede a diferenca de altura ao redor da celula.
//
// ----------------------------
float GetSlopeScore(
    const TerrainPlane &terrain,
    int x,
    int z,
    float height)
{
    const float east  = std::fabs(GetCellHeight(terrain, x + 1, z) - height);
    const float west  = std::fabs(GetCellHeight(terrain, x - 1, z) - height);
    const float south = std::fabs(GetCellHeight(terrain, x, z + 1) - height);
    const float north = std::fabs(GetCellHeight(terrain, x, z - 1) - height);

    return std::max(std::max(east, west), std::max(south, north));
}

// ----------------------------
// HasRockGround
//
// Decide se uma celula e um bom ponto para receber rochas.
//
// ----------------------------
bool HasRockGround(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    if (
        terrain.mapKind != TerrainMapKind::LowPolyWorld ||
        x <= 0 ||
        z <= 0 ||
        x >= terrain.widthCells - 1 ||
        z >= terrain.heightCells - 1)
    {
        return false;
    }

    const float height = GetCellHeight(terrain, x, z);
    if (height <= terrain.waterLevel + minimumRockClearance)
    {
        return false;
    }

    const float slopeScore = GetSlopeScore(terrain, x, z, height);
    return slopeScore >= 0.18f || height > terrain.maximumHeight * 0.42f;
}

// ----------------------------
// GetDesiredRockCount
//
// Calcula quantos grupos de rochas o mapa deve tentar distribuir.
//
// ----------------------------
int GetDesiredRockCount(const TerrainPlane &terrain)
{
    const int cellCount = terrain.widthCells * terrain.heightCells;
    return std::clamp(cellCount / 70, 6, maximumRockClusterCount);
}

// ----------------------------
// GetTerrainStart
//
// Retorna o canto inicial do terreno no plano XZ.
//
// ----------------------------
Vector2 GetTerrainStart(const TerrainPlane &terrain)
{
    const float width  = static_cast<float>(terrain.widthCells) * terrain.cellSize;
    const float height = static_cast<float>(terrain.heightCells) * terrain.cellSize;

    return {
        terrain.origin.x - width * 0.5f,
        terrain.origin.z - height * 0.5f
    };
}

// ----------------------------
// GetRockColor
//
// Escolhe uma variacao de cinza/verde para a pedra.
//
// ----------------------------
Color GetRockColor(
    unsigned int seed,
    int x,
    int z,
    int salt)
{
    const float colorChoice = Hash01(seed, x, z, salt);

    if (colorChoice < 0.28f)
    {
        return rockLightColor;
    }

    if (colorChoice < 0.72f)
    {
        return rockMidColor;
    }

    return rockShadowColor;
}

// ----------------------------
// GetRockCenter
//
// Calcula a posicao de uma pedra dentro de uma celula.
//
// ----------------------------
Vector3 GetRockCenter(
    const TerrainPlane &terrain,
    int x,
    int z,
    int salt)
{
    const Vector2 start     = GetTerrainStart(terrain);
    const float jitterLimit = terrain.cellSize * 0.28f;
    const float jitterX     = RandomRange(
        terrain.seed,
        x,
        z,
        30 + salt,
        -jitterLimit,
        jitterLimit);
    const float jitterZ     = RandomRange(
        terrain.seed,
        x,
        z,
        40 + salt,
        -jitterLimit,
        jitterLimit);

    return {
        start.x + (static_cast<float>(x) + 0.5f) * terrain.cellSize + jitterX,
        terrain.origin.y + GetCellHeight(terrain, x, z) + groundOffset,
        start.y + (static_cast<float>(z) + 0.5f) * terrain.cellSize + jitterZ
    };
}

// ----------------------------
// CreateStone
//
// Cria uma pedra low-poly com escala irregular.
//
// ----------------------------
ProceduralStone CreateStone(
    const TerrainPlane &terrain,
    int x,
    int z,
    int salt)
{
    const float baseScale = RandomRange(
        terrain.seed,
        x,
        z,
        50 + salt,
        terrain.cellSize * 0.18f,
        terrain.cellSize * 0.42f);

    return {
        GetRockCenter(terrain, x, z, salt),
        {
            baseScale * RandomRange(terrain.seed, x, z, 60 + salt, 0.9f, 1.55f),
            baseScale * RandomRange(terrain.seed, x, z, 70 + salt, 0.38f, 0.78f),
            baseScale * RandomRange(terrain.seed, x, z, 80 + salt, 0.9f, 1.55f)
        },
        RandomRange(terrain.seed, x, z, 90 + salt, 0.0f, 360.0f),
        GetRockColor(terrain.seed, x, z, 100 + salt)
    };
}

// ----------------------------
// CreateRockCluster
//
// Gera um grupo procedural de rochas em uma celula.
//
// ----------------------------
ProceduralRockCluster CreateRockCluster(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    ProceduralRockCluster cluster = {};
    const int stoneCount          = 1 + static_cast<int>(
        HashCell(terrain.seed, x, z, 7) % 3u);

    cluster.stones.reserve(static_cast<std::size_t>(stoneCount));

    for (int stone = 0; stone < stoneCount; ++stone)
    {
        cluster.stones.push_back(CreateStone(terrain, x, z, stone));
    }

    return cluster;
}

// ----------------------------
// StoreRockFingerprint
//
// Guarda os valores do terreno que definem as rochas atuais.
//
// ----------------------------
void StoreRockFingerprint(
    ProceduralRocks *rocks,
    const TerrainPlane &terrain)
{
    rocks->widthCells    = terrain.widthCells;
    rocks->heightCells   = terrain.heightCells;
    rocks->maximumHeight = terrain.maximumHeight;
    rocks->lakeDepth     = terrain.lakeDepth;
    rocks->noiseScale    = terrain.noiseScale;
    rocks->waterLevel    = terrain.waterLevel;
    rocks->seed          = terrain.seed;
    rocks->mapKind       = terrain.mapKind;
}

// ----------------------------
// NeedsRockRegeneration
//
// Verifica se o terreno mudou desde as ultimas rochas geradas.
//
// ----------------------------
bool NeedsRockRegeneration(
    const ProceduralRocks &rocks,
    const TerrainPlane &terrain)
{
    return rocks.widthCells != terrain.widthCells ||
        rocks.heightCells != terrain.heightCells ||
        rocks.maximumHeight != terrain.maximumHeight ||
        rocks.lakeDepth != terrain.lakeDepth ||
        rocks.noiseScale != terrain.noiseScale ||
        rocks.waterLevel != terrain.waterLevel ||
        rocks.seed != terrain.seed ||
        rocks.mapKind != terrain.mapKind;
}

// ----------------------------
// CollectRockCandidates
//
// Seleciona celulas candidatas para grupos de rochas.
//
// ----------------------------
std::vector<RockCandidate> CollectRockCandidates(const TerrainPlane &terrain)
{
    std::vector<RockCandidate> candidates;
    candidates.reserve(
        static_cast<std::size_t>(
            terrain.widthCells * terrain.heightCells / 5));

    for (int z = 0; z < terrain.heightCells; ++z)
    {
        for (int x = 0; x < terrain.widthCells; ++x)
        {
            if (!HasRockGround(terrain, x, z))
            {
                continue;
            }

            const float score = Hash01(terrain.seed, x, z, 11);
            if (score < 0.34f)
            {
                candidates.push_back({ x, z, score });
            }
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const RockCandidate &left, const RockCandidate &right)
        {
            return left.score < right.score;
        });

    return candidates;
}

// ----------------------------
// DrawStone
//
// Desenha uma pedra low-poly com transformacao irregular.
//
// ----------------------------
void DrawStone(
    const ProceduralStone &stone,
    bool showMesh)
{
    rlPushMatrix();
    rlTranslatef(stone.center.x, stone.center.y, stone.center.z);
    rlRotatef(stone.rotationY, 0.0f, 1.0f, 0.0f);
    rlScalef(stone.scale.x, stone.scale.y, stone.scale.z);
    DrawSphereEx(
        { 0.0f, 0.0f, 0.0f },
        1.0f,
        rockRings,
        rockSlices,
        stone.color);
    if (showMesh)
    {
        DrawSphereWires(
            { 0.0f, 0.0f, 0.0f },
            1.0f,
            rockRings,
            rockSlices,
            rockMeshColor);
    }
    rlPopMatrix();
}

// ----------------------------
// DrawRockCluster
//
// Desenha todas as pedras de um grupo.
//
// ----------------------------
void DrawRockCluster(
    const ProceduralRockCluster &cluster,
    bool showMesh)
{
    for (const ProceduralStone &stone : cluster.stones)
    {
        DrawStone(stone, showMesh);
    }
}
}

// ----------------------------
// CreateProceduralRocks
//
// Cria uma colecao vazia de rochas procedurais.
//
// ----------------------------
ProceduralRocks CreateProceduralRocks()
{
    ProceduralRocks rocks = {};
    rocks.widthCells      = -1;
    rocks.heightCells     = -1;
    rocks.maximumHeight   = -1.0f;
    rocks.lakeDepth       = -1.0f;
    rocks.noiseScale      = -1.0f;
    rocks.waterLevel      = -1.0f;
    rocks.seed            = 0u;
    rocks.mapKind         = TerrainMapKind::FlatGrid;
    return rocks;
}

// ----------------------------
// UpdateProceduralRocks
//
// Regenera as rochas quando o terreno muda.
//
// ----------------------------
void UpdateProceduralRocks(
    ProceduralRocks *rocks,
    const TerrainPlane &terrain)
{
    if (!terrain.created || terrain.cellHeights.empty())
    {
        rocks->clusters.clear();
        return;
    }

    if (!NeedsRockRegeneration(*rocks, terrain))
    {
        return;
    }

    StoreRockFingerprint(rocks, terrain);
    rocks->clusters.clear();

    if (terrain.mapKind != TerrainMapKind::LowPolyWorld)
    {
        return;
    }

    std::vector<RockCandidate> candidates = CollectRockCandidates(terrain);
    const int rockCount                   = std::min(
        GetDesiredRockCount(terrain),
        static_cast<int>(candidates.size()));

    rocks->clusters.reserve(static_cast<std::size_t>(rockCount));

    for (int index = 0; index < rockCount; ++index)
    {
        const RockCandidate &candidate =
            candidates[static_cast<std::size_t>(index)];
        rocks->clusters.push_back(
            CreateRockCluster(terrain, candidate.x, candidate.z));
    }
}

// ----------------------------
// DrawProceduralRocks
//
// Desenha todos os grupos de rochas procedurais.
//
// ----------------------------
void DrawProceduralRocks(
    const ProceduralRocks &rocks,
    bool showMesh)
{
    for (const ProceduralRockCluster &cluster : rocks.clusters)
    {
        DrawRockCluster(cluster, showMesh);
    }
}
