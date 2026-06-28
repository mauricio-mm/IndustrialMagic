#ifndef TERRAIN_PANEL_H
#define TERRAIN_PANEL_H

#include "terrain_plane.h"

#include <array>
#include <string>

struct TerrainPanel
{
    std::array<std::string, 2> values;
    int activeField;
    bool replaceOnNextInput;
};

TerrainPanel CreateTerrainPanel();
bool UpdateTerrainPanel(
    TerrainPanel *panel,
    TerrainPlane *terrain);
void DrawTerrainPanel(
    const TerrainPanel &panel,
    const TerrainPlane &terrain);

#endif
