#include "terrain_internal.h"
#include "terrain/terrain_plane.h"

// ----------------------------
// DrawTerrainPlane
//
// Percorre as celulas e desenha topo, laterais expostas, agua e conectores.
//
// ----------------------------
void DrawTerrainPlane(
    const TerrainPlane &terrain,
    const Camera3D &camera)
{
    if (!terrain.created || terrain.cellHeights.empty())
    {
        return;
    }

    const float width   = static_cast<float>(terrain.widthCells) * terrain.cellSize;
    const float height  = static_cast<float>(terrain.heightCells) * terrain.cellSize;
    const float startX  = terrain.origin.x - width * 0.5f;
    const float startZ  = terrain.origin.z - height * 0.5f;
    const float bottomY = terrain_internal::GetTerrainBottom(terrain);

    for (int z = 0; z < terrain.heightCells; ++z)
    {
        for (int x = 0; x < terrain.widthCells; ++x)
        {
            const float currentHeight = terrain_internal::GetCellHeight(terrain, x, z);
            const float lowTopY       = terrain.origin.y + currentHeight;
            const float minX          = startX + static_cast<float>(x) * terrain.cellSize;
            const float maxX          = minX + terrain.cellSize;
            const float minZ          = startZ + static_cast<float>(z) * terrain.cellSize;
            const float maxZ          = minZ + terrain.cellSize;

            Vector3 northWest         = {};
            Vector3 northEast         = {};
            Vector3 southEast         = {};
            Vector3 southWest         = {};
            terrain_internal::GetCellCorners(
                startX,
                startZ,
                terrain.cellSize,
                x,
                z,
                lowTopY + terrain_internal::topOffset,
                lowTopY + terrain_internal::topOffset,
                lowTopY + terrain_internal::topOffset,
                lowTopY + terrain_internal::topOffset,
                &northWest,
                &northEast,
                &southEast,
                &southWest);

            if (terrain_internal::IsTopVisibleFromCamera(camera, lowTopY))
            {
                terrain_internal::DrawTopTile(
                    northWest,
                    northEast,
                    southEast,
                    southWest,
                    terrain_internal::GetTopColor(terrain, currentHeight));

                if (terrain.showMesh)
                {
                    terrain_internal::DrawTopMesh(
                        northWest,
                        northEast,
                        southEast,
                        southWest);
                }
            }

            if (terrain_internal::IsSideExposed(
                    terrain,
                    x,
                    z - 1,
                    currentHeight))
            {
                terrain_internal::DrawExposedSide(
                    camera,
                    terrain_internal::TerrainSide::North,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + terrain_internal::topOffset,
                    terrain_internal::GetCellBottomY(
                        terrain,
                        bottomY,
                        x,
                        z - 1),
                    terrain.showMesh);
            }

            if (terrain_internal::IsSideExposed(
                    terrain,
                    x + 1,
                    z,
                    currentHeight))
            {
                terrain_internal::DrawExposedSide(
                    camera,
                    terrain_internal::TerrainSide::East,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + terrain_internal::topOffset,
                    terrain_internal::GetCellBottomY(
                        terrain,
                        bottomY,
                        x + 1,
                        z),
                    terrain.showMesh);
            }

            if (terrain_internal::IsSideExposed(
                    terrain,
                    x,
                    z + 1,
                    currentHeight))
            {
                terrain_internal::DrawExposedSide(
                    camera,
                    terrain_internal::TerrainSide::South,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + terrain_internal::topOffset,
                    terrain_internal::GetCellBottomY(
                        terrain,
                        bottomY,
                        x,
                        z + 1),
                    terrain.showMesh);
            }

            if (terrain_internal::IsSideExposed(
                    terrain,
                    x - 1,
                    z,
                    currentHeight))
            {
                terrain_internal::DrawExposedSide(
                    camera,
                    terrain_internal::TerrainSide::West,
                    minX,
                    maxX,
                    minZ,
                    maxZ,
                    lowTopY + terrain_internal::topOffset,
                    terrain_internal::GetCellBottomY(
                        terrain,
                        bottomY,
                        x - 1,
                        z),
                    terrain.showMesh);
            }

            const terrain_internal::HalfCubeConnector connector =
                terrain_internal::GetHalfCubeConnector(terrain, x, z);
            if (connector.corner != terrain_internal::HalfCubeCorner::None)
            {
                terrain_internal::DrawHalfCubeConnector(
                    connector,
                    northWest,
                    northEast,
                    southEast,
                    southWest,
                    terrain.origin.y + connector.height +
                        terrain_internal::topOffset,
                    terrain.showMesh);
            }

            if (terrain.mapKind == TerrainMapKind::LowPolyWorld &&
                lowTopY < terrain.origin.y + terrain.waterLevel)
            {
                terrain_internal::DrawWaterTile(
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
        { startX, terrain.origin.y + terrain_internal::meshOffset, startZ },
        {
            startX + width,
            terrain.origin.y + terrain_internal::meshOffset,
            startZ
        },
        terrain_internal::edgeColor);
    DrawLine3D(
        {
            startX + width,
            terrain.origin.y + terrain_internal::meshOffset,
            startZ
        },
        {
            startX + width,
            terrain.origin.y + terrain_internal::meshOffset,
            startZ + height
        },
        terrain_internal::edgeColor);
    DrawLine3D(
        {
            startX + width,
            terrain.origin.y + terrain_internal::meshOffset,
            startZ + height
        },
        {
            startX,
            terrain.origin.y + terrain_internal::meshOffset,
            startZ + height
        },
        terrain_internal::edgeColor);
    DrawLine3D(
        {
            startX,
            terrain.origin.y + terrain_internal::meshOffset,
            startZ + height
        },
        { startX, terrain.origin.y + terrain_internal::meshOffset, startZ },
        terrain_internal::edgeColor);
}
