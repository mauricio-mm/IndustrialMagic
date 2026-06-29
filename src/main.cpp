#include "environment3d.h"
#include "raylib.h"
#include "terrain/terrain_panel.h"
#include "terrain/terrain_plane.h"

// ----------------------------
// main
//
// Inicializa a janela, atualiza a cena e coordena desenho de UI e terreno.
//
// ----------------------------
int main(void)
{
    SetConfigFlags(
        FLAG_MSAA_4X_HINT |
        FLAG_WINDOW_RESIZABLE |
        FLAG_WINDOW_HIGHDPI);
    InitWindow(1280, 720, "IndustrialMagic");
    SetTargetFPS(60);

    Environment3D environment = CreateEnvironment3D();
    TerrainPlane terrain      = CreateTerrainPlane();
    TerrainPanel terrainPanel = CreateTerrainPanel();
    
    InitEnvironment3D();

    while (!WindowShouldClose())
    {
        const bool terrainPanelCapturesInput = UpdateTerrainPanel(&terrainPanel, &terrain);
        if (terrainPanel.tabPressed)
        {
            terrain.showMesh = !terrain.showMesh;
        }

        const bool editingNoiseScale = IsKeyDown(KEY_R);

        if (editingNoiseScale)
        {
            AdjustTerrainNoiseScale(&terrain, GetMouseWheelMove());
        }

        UpdateEnvironment3D(
            &environment,
            terrainPanelCapturesInput || editingNoiseScale);

        BeginDrawing();
        DrawSkyBackground();

        BeginMode3D(environment.camera);
        DrawTerrainPlane(terrain, environment.camera);
        DrawEnvironment3D(environment);
        EndMode3D();

        DrawTerrainPanel(terrainPanel, terrain);

        EndDrawing();
    }

    ShutdownEnvironment3D();
    CloseWindow();
    return 0;
}
