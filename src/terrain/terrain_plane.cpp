#include "terrain_internal.h"
#include "terrain/terrain_plane.h"

#include <algorithm>
#include <cstddef>

// ----------------------------
// CreateTerrainPlane
//
// Cria o terreno com valores padrao e gera as alturas iniciais.
//
// ----------------------------
TerrainPlane CreateTerrainPlane()
{
    TerrainPlane terrain = {};
    terrain.origin        = { 0.0f, 0.0f, 0.0f };
    terrain.widthCells    = 32;
    terrain.heightCells   = 32;
    terrain.cellSize      = 1.0f;
    terrain.maximumHeight = 15.0f;
    terrain.lakeDepth     = 4.0f;
    terrain.waterLevel    = 0.0f;
    terrain.noiseScale    = 1.0f;
    terrain.seed          = 1337u;
    terrain.color         = { 220, 224, 218, 255 };
    terrain.created       = true;
    terrain.showMesh      = false;
    terrain.mapKind       = TerrainMapKind::LowPolyWorld;
    GenerateTerrainPlane(&terrain);
    return terrain;
}

// ----------------------------
// GenerateTerrainPlane
//
// Recalcula a matriz de alturas do terreno usando o mapa atual.
//
// ----------------------------
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

    const float width  = static_cast<float>(terrain->widthCells) * terrain->cellSize;
    const float height = static_cast<float>(terrain->heightCells) * terrain->cellSize;
    const float startX = terrain->origin.x - width * 0.5f;
    const float startZ = terrain->origin.z - height * 0.5f;

    for (int z = 0; z < terrain->heightCells; ++z)
    {
        for (int x = 0; x < terrain->widthCells; ++x)
        {
            const float worldX = startX + (static_cast<float>(x) + 0.5f) * terrain->cellSize;
            const float worldZ = startZ + (static_cast<float>(z) + 0.5f) * terrain->cellSize;

            terrain->cellHeights[
                terrain_internal::GetCellIndex(*terrain, x, z)] =
                terrain_internal::GetTerrainHeight(
                    worldX,
                    worldZ,
                    *terrain);
        }
    }

    terrain->created = true;
}

// ----------------------------
// AdjustTerrainNoiseScale
//
// Ajusta a escala do noise com o scroll e regenera o mapa.
//
// ----------------------------
void AdjustTerrainNoiseScale(
    TerrainPlane *terrain,
    float wheelMove)
{
    if (wheelMove == 0.0f)
    {
        return;
    }

    terrain->noiseScale = std::clamp(
        terrain->noiseScale + wheelMove * terrain_internal::noiseScaleStep,
        terrain_internal::minimumNoiseScale,
        terrain_internal::maximumNoiseScale);
    GenerateTerrainPlane(terrain);
}

// ----------------------------
// SetTerrainMapKind
//
// Troca o tipo de mapa ativo e regenera o terreno.
//
// ----------------------------
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

// ----------------------------
// GetTerrainMapName
//
// Retorna o nome legivel do tipo de mapa atual.
//
// ----------------------------
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
