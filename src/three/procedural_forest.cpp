#include "three/procedural_forest.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace
{
constexpr float fullTurnRadians      = 6.28318530718f;
constexpr float treeGroundOffset     = 0.035f;
constexpr float minimumTreeClearance = 0.18f;
constexpr int maximumTreeCount       = 220;
constexpr int trunkSides             = 6;
constexpr int branchSides            = 5;
constexpr int leafRings              = 5;
constexpr int leafSlices             = 7;

constexpr Color trunkColor      = { 91, 58, 36, 255 };
constexpr Color branchColor     = { 77, 48, 31, 255 };
constexpr Color leafLightColor  = { 110, 150, 82, 255 };
constexpr Color leafMidColor    = { 82, 126, 76, 255 };
constexpr Color leafShadowColor = { 63, 96, 69, 255 };

struct TreeCandidate
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
// HasTreeGround
//
// Decide se uma celula e um bom ponto para receber uma arvore.
//
// ----------------------------
bool HasTreeGround(
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
    if (height <= terrain.waterLevel + minimumTreeClearance)
    {
        return false;
    }

    const float maximumSlope = std::max(1.15f, terrain.maximumHeight * 0.12f);
    return std::fabs(GetCellHeight(terrain, x + 1, z) - height) <=
            maximumSlope &&
        std::fabs(GetCellHeight(terrain, x - 1, z) - height) <=
            maximumSlope &&
        std::fabs(GetCellHeight(terrain, x, z + 1) - height) <=
            maximumSlope &&
        std::fabs(GetCellHeight(terrain, x, z - 1) - height) <=
            maximumSlope;
}

// ----------------------------
// GetDesiredTreeCount
//
// Calcula quantas arvores o mapa deve tentar distribuir.
//
// ----------------------------
int GetDesiredTreeCount(const TerrainPlane &terrain)
{
    const int cellCount = terrain.widthCells * terrain.heightCells;
    return std::clamp(cellCount / 55, 8, maximumTreeCount);
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
// GetTreePosition
//
// Calcula a posicao final da arvore sobre uma celula.
//
// ----------------------------
Vector3 GetTreePosition(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    const Vector2 start     = GetTerrainStart(terrain);
    const float jitterLimit = terrain.cellSize * 0.24f;
    const float jitterX     = RandomRange(
        terrain.seed,
        x,
        z,
        20,
        -jitterLimit,
        jitterLimit);
    const float jitterZ     = RandomRange(
        terrain.seed,
        x,
        z,
        21,
        -jitterLimit,
        jitterLimit);

    return {
        start.x + (static_cast<float>(x) + 0.5f) * terrain.cellSize + jitterX,
        terrain.origin.y + GetCellHeight(terrain, x, z) + treeGroundOffset,
        start.y + (static_cast<float>(z) + 0.5f) * terrain.cellSize + jitterZ
    };
}

// ----------------------------
// GetLeafColor
//
// Escolhe uma variacao de verde para uma esfera de folhas.
//
// ----------------------------
Color GetLeafColor(
    unsigned int seed,
    int x,
    int z,
    int salt)
{
    const float colorChoice = Hash01(seed, x, z, salt);

    if (colorChoice < 0.32f)
    {
        return leafLightColor;
    }

    if (colorChoice < 0.74f)
    {
        return leafMidColor;
    }

    return leafShadowColor;
}

// ----------------------------
// AddBranch
//
// Adiciona um cilindro procedural aos galhos da arvore.
//
// ----------------------------
void AddBranch(
    ProceduralTree *tree,
    Vector3 start,
    Vector3 end,
    float startRadius,
    float endRadius)
{
    tree->branches.push_back({
        start,
        end,
        startRadius,
        endRadius
    });
}

// ----------------------------
// AddLeaf
//
// Adiciona uma esfera de folhas procedural a arvore.
//
// ----------------------------
void AddLeaf(
    ProceduralTree *tree,
    Vector3 center,
    float radius,
    Color color)
{
    tree->leaves.push_back({
        center,
        radius,
        color
    });
}

// ----------------------------
// CreateTree
//
// Gera tronco, galhos e esferas de uma arvore em uma celula.
//
// ----------------------------
ProceduralTree CreateTree(
    const TerrainPlane &terrain,
    int x,
    int z)
{
    ProceduralTree tree = {};
    tree.trunkStart     = GetTreePosition(terrain, x, z);
    tree.trunkRadius    = RandomRange(
        terrain.seed,
        x,
        z,
        30,
        terrain.cellSize * 0.055f,
        terrain.cellSize * 0.095f);

    const float trunkHeight = RandomRange(
        terrain.seed,
        x,
        z,
        31,
        terrain.cellSize * 1.55f,
        terrain.cellSize * 2.55f);
    const float leanX       = RandomRange(
        terrain.seed,
        x,
        z,
        32,
        -terrain.cellSize * 0.12f,
        terrain.cellSize * 0.12f);
    const float leanZ       = RandomRange(
        terrain.seed,
        x,
        z,
        33,
        -terrain.cellSize * 0.12f,
        terrain.cellSize * 0.12f);

    tree.trunkEnd = {
        tree.trunkStart.x + leanX,
        tree.trunkStart.y + trunkHeight,
        tree.trunkStart.z + leanZ
    };

    const int branchCount = 3 + static_cast<int>(
        HashCell(terrain.seed, x, z, 34) % 3u);

    for (int branch = 0; branch < branchCount; ++branch)
    {
        const float branchRatio = 0.42f +
            static_cast<float>(branch) /
                static_cast<float>(branchCount) * 0.46f;
        const float angle       = RandomRange(
            terrain.seed,
            x,
            z,
            40 + branch,
            0.0f,
            fullTurnRadians) +
            static_cast<float>(branch) * 1.71f;
        const float length      = RandomRange(
            terrain.seed,
            x,
            z,
            50 + branch,
            terrain.cellSize * 0.45f,
            terrain.cellSize * 0.82f);
        const Vector3 start     = {
            tree.trunkStart.x +
                (tree.trunkEnd.x - tree.trunkStart.x) * branchRatio,
            tree.trunkStart.y + trunkHeight * branchRatio,
            tree.trunkStart.z +
                (tree.trunkEnd.z - tree.trunkStart.z) * branchRatio
        };
        const Vector3 end       = {
            start.x + std::cos(angle) * length,
            start.y + RandomRange(
                terrain.seed,
                x,
                z,
                60 + branch,
                terrain.cellSize * 0.15f,
                terrain.cellSize * 0.36f),
            start.z + std::sin(angle) * length
        };

        AddBranch(
            &tree,
            start,
            end,
            tree.trunkRadius * 0.62f,
            tree.trunkRadius * 0.24f);
        AddLeaf(
            &tree,
            end,
            RandomRange(
                terrain.seed,
                x,
                z,
                70 + branch,
                terrain.cellSize * 0.35f,
                terrain.cellSize * 0.54f),
            GetLeafColor(terrain.seed, x, z, 80 + branch));
    }

    AddLeaf(
        &tree,
        {
            tree.trunkEnd.x,
            tree.trunkEnd.y + terrain.cellSize * 0.18f,
            tree.trunkEnd.z
        },
        RandomRange(
            terrain.seed,
            x,
            z,
            90,
            terrain.cellSize * 0.48f,
            terrain.cellSize * 0.66f),
        GetLeafColor(terrain.seed, x, z, 91));

    return tree;
}

// ----------------------------
// StoreForestFingerprint
//
// Guarda os valores do terreno que definem a floresta atual.
//
// ----------------------------
void StoreForestFingerprint(
    ProceduralForest *forest,
    const TerrainPlane &terrain)
{
    forest->widthCells    = terrain.widthCells;
    forest->heightCells   = terrain.heightCells;
    forest->maximumHeight = terrain.maximumHeight;
    forest->lakeDepth     = terrain.lakeDepth;
    forest->noiseScale    = terrain.noiseScale;
    forest->waterLevel    = terrain.waterLevel;
    forest->seed          = terrain.seed;
    forest->mapKind       = terrain.mapKind;
}

// ----------------------------
// NeedsForestRegeneration
//
// Verifica se o terreno mudou desde a ultima floresta gerada.
//
// ----------------------------
bool NeedsForestRegeneration(
    const ProceduralForest &forest,
    const TerrainPlane &terrain)
{
    return forest.widthCells != terrain.widthCells ||
        forest.heightCells != terrain.heightCells ||
        forest.maximumHeight != terrain.maximumHeight ||
        forest.lakeDepth != terrain.lakeDepth ||
        forest.noiseScale != terrain.noiseScale ||
        forest.waterLevel != terrain.waterLevel ||
        forest.seed != terrain.seed ||
        forest.mapKind != terrain.mapKind;
}

// ----------------------------
// CollectTreeCandidates
//
// Seleciona celulas candidatas para receber arvores.
//
// ----------------------------
std::vector<TreeCandidate> CollectTreeCandidates(const TerrainPlane &terrain)
{
    std::vector<TreeCandidate> candidates;
    candidates.reserve(
        static_cast<std::size_t>(
            terrain.widthCells * terrain.heightCells / 4));

    for (int z = 0; z < terrain.heightCells; ++z)
    {
        for (int x = 0; x < terrain.widthCells; ++x)
        {
            if (!HasTreeGround(terrain, x, z))
            {
                continue;
            }

            const float score = Hash01(terrain.seed, x, z, 10);
            if (score < 0.42f)
            {
                candidates.push_back({ x, z, score });
            }
        }
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const TreeCandidate &left, const TreeCandidate &right)
        {
            return left.score < right.score;
        });

    return candidates;
}

// ----------------------------
// DrawTree
//
// Desenha tronco, galhos e esferas de folhas de uma arvore.
//
// ----------------------------
void DrawTree(const ProceduralTree &tree)
{
    DrawCylinderEx(
        tree.trunkStart,
        tree.trunkEnd,
        tree.trunkRadius,
        tree.trunkRadius * 0.58f,
        trunkSides,
        trunkColor);

    for (const ProceduralBranch &branch : tree.branches)
    {
        DrawCylinderEx(
            branch.start,
            branch.end,
            branch.startRadius,
            branch.endRadius,
            branchSides,
            branchColor);
    }

    for (const ProceduralLeaf &leaf : tree.leaves)
    {
        DrawSphereEx(
            leaf.center,
            leaf.radius,
            leafRings,
            leafSlices,
            leaf.color);
    }
}
}

// ----------------------------
// CreateProceduralForest
//
// Cria uma floresta vazia aguardando o primeiro terreno valido.
//
// ----------------------------
ProceduralForest CreateProceduralForest()
{
    ProceduralForest forest = {};
    forest.widthCells       = -1;
    forest.heightCells      = -1;
    forest.maximumHeight    = -1.0f;
    forest.lakeDepth        = -1.0f;
    forest.noiseScale       = -1.0f;
    forest.waterLevel       = -1.0f;
    forest.seed             = 0u;
    forest.mapKind          = TerrainMapKind::FlatGrid;
    return forest;
}

// ----------------------------
// UpdateProceduralForest
//
// Regenera a floresta quando o terreno muda.
//
// ----------------------------
void UpdateProceduralForest(
    ProceduralForest *forest,
    const TerrainPlane &terrain)
{
    if (!terrain.created || terrain.cellHeights.empty())
    {
        forest->trees.clear();
        return;
    }

    if (!NeedsForestRegeneration(*forest, terrain))
    {
        return;
    }

    StoreForestFingerprint(forest, terrain);
    forest->trees.clear();

    if (terrain.mapKind != TerrainMapKind::LowPolyWorld)
    {
        return;
    }

    std::vector<TreeCandidate> candidates = CollectTreeCandidates(terrain);
    const int treeCount                   = std::min(
        GetDesiredTreeCount(terrain),
        static_cast<int>(candidates.size()));

    forest->trees.reserve(static_cast<std::size_t>(treeCount));

    for (int index = 0; index < treeCount; ++index)
    {
        const TreeCandidate &candidate =
            candidates[static_cast<std::size_t>(index)];
        forest->trees.push_back(
            CreateTree(terrain, candidate.x, candidate.z));
    }
}

// ----------------------------
// DrawProceduralForest
//
// Desenha todas as arvores procedurais da floresta.
//
// ----------------------------
void DrawProceduralForest(const ProceduralForest &forest)
{
    for (const ProceduralTree &tree : forest.trees)
    {
        DrawTree(tree);
    }
}
