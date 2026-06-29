#include "terrain_internal.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace terrain_internal
{
// ----------------------------
// GetCellIndex
//
// Converte coordenadas da grade em indice linear do vetor de alturas.
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

namespace
{
// ----------------------------
// HashCell
//
// Gera um hash deterministico para uma celula do noise.
//
// ----------------------------
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

// ----------------------------
// HashToUnitFloat
//
// Converte o hash da celula para um valor normalizado entre 0 e 1.
//
// ----------------------------
float HashToUnitFloat(
    int x,
    int z,
    unsigned int seed)
{
    constexpr float divisor = 16777215.0f;
    return static_cast<float>(
        HashCell(x, z, seed) & 0x00FFFFFFu) / divisor;
}

// ----------------------------
// SmoothStep
//
// Suaviza uma interpolacao para evitar quebras duras no noise.
//
// ----------------------------
float SmoothStep(float value)
{
    return value * value * (3.0f - 2.0f * value);
}

// ----------------------------
// Lerp
//
// Interpola linearmente entre dois valores.
//
// ----------------------------
float Lerp(float start, float end, float amount)
{
    return start + (end - start) * amount;
}

// ----------------------------
// ValueNoise
//
// Amostra noise de valor 2D com interpolacao suave.
//
// ----------------------------
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

    const float top    = Lerp(
        HashToUnitFloat(x0, z0, seed),
        HashToUnitFloat(x1, z0, seed),
        tx);
    const float bottom = Lerp(
        HashToUnitFloat(x0, z1, seed),
        HashToUnitFloat(x1, z1, seed),
        tx);
    return Lerp(top, bottom, tz);
}

// ----------------------------
// MaskAbove
//
// Cria uma mascara suave quando um valor passa de um limite.
//
// ----------------------------
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
}

// ----------------------------
// GetTerraceStep
//
// Calcula o tamanho do degrau usado para quantizar as alturas.
//
// ----------------------------
float GetTerraceStep(const TerrainPlane &terrain)
{
    return std::max(minimumTerraceStep, terrain.maximumHeight / 28.0f);
}

// ----------------------------
// GetTerrainHeight
//
// Combina camadas de noise para gerar altura, montanhas e bacias.
//
// ----------------------------
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
    const float base    =
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

// ----------------------------
// GetTerrainBottom
//
// Retorna a base visual minima usada para laterais e lagos.
//
// ----------------------------
float GetTerrainBottom(const TerrainPlane &terrain)
{
    return terrain.origin.y - terrain.lakeDepth - 1.0f;
}

// ----------------------------
// GetCellHeight
//
// Le a altura de uma celula, tratando vizinhos fora do terreno como baixos.
//
// ----------------------------
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

// ----------------------------
// GetCellBottomY
//
// Calcula de onde uma face lateral deve comecar em relacao ao vizinho.
//
// ----------------------------
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

} // namespace terrain_internal
