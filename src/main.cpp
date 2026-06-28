#include "environment3d.h"
#include "raylib.h"
#include "terrain_panel.h"
#include "terrain_plane.h"

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
        UpdateEnvironment3D(&environment, terrainPanelCapturesInput);

        BeginDrawing();
        ClearBackground(RAYWHITE);

        BeginMode3D(environment.camera);
        DrawTerrainPlane(terrain);
        DrawEnvironment3D(environment);
        EndMode3D();

        DrawTerrainPanel(terrainPanel, terrain);

        EndDrawing();
    }

    ShutdownEnvironment3D();
    CloseWindow();
    return 0;
}
