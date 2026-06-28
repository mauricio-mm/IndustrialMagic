#ifndef ENVIRONMENT3D_H
#define ENVIRONMENT3D_H

#include "raylib.h"

enum class CameraMouseControl
{
    None,
    Rotate,
    Pan
};

struct Environment3D
{
    Camera3D camera;
    int gridHalfSize;
    float gridSpacing;
    CameraMouseControl mouseControl;
};

Environment3D CreateEnvironment3D();
void InitEnvironment3D();
void UpdateEnvironment3D(
    Environment3D *environment,
    bool blockMouseInput);
void DrawEnvironment3D(const Environment3D &environment);
void ShutdownEnvironment3D();

#endif
