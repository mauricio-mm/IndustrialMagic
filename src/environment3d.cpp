#include "environment3d.h"

namespace
{
constexpr Color skyTopColor = { 93, 167, 232, 255 };
constexpr Color skyHorizonColor = { 203, 229, 248, 255 };
constexpr Color sunCoreColor = { 255, 241, 177, 210 };
constexpr Color sunHaloColor = { 255, 241, 177, 0 };
constexpr Color gridColor = { 215, 218, 222, 255 };
constexpr float axisHeight = 5.0f;
constexpr float cameraMoveSpeed = 5.4f;
constexpr float mouseSensitivity = 0.172f;
constexpr float mousePanSensitivity = 0.025f;
}

// ----------------------------
// CreateEnvironment3D
//
// Cria a camera e os parametros basicos do ambiente 3D.
//
// ----------------------------
Environment3D CreateEnvironment3D()
{
    Environment3D environment = {};
    environment.camera.position   = { 10.0f, 8.0f, 10.0f };
    environment.camera.target     = { 0.0f, 0.0f, 0.0f };
    environment.camera.up         = { 0.0f, 1.0f, 0.0f };
    environment.camera.fovy       = 45.0f;
    environment.camera.projection = CAMERA_PERSPECTIVE;
    environment.gridHalfSize      = 10;
    environment.gridSpacing       = 1.0f;
    environment.mouseControl      = CameraMouseControl::None;
    return environment;
}

// ----------------------------
// InitEnvironment3D
//
// Inicializa o estado de cursor usado pelo ambiente.
//
// ----------------------------
void InitEnvironment3D()
{
    EnableCursor();
}

// ----------------------------
// UpdateEnvironment3D
//
// Atualiza movimentacao, rotacao, zoom e captura de mouse da camera.
//
// ----------------------------
void UpdateEnvironment3D(Environment3D *environment, bool blockMouseInput)
{
    CameraMouseControl requestedControl = CameraMouseControl::None;

    if (!blockMouseInput &&
        IsMouseButtonDown(MOUSE_BUTTON_LEFT))
    {
        requestedControl = CameraMouseControl::Rotate;
    }
    else if (!blockMouseInput &&
             IsMouseButtonDown(MOUSE_BUTTON_RIGHT))
    {
        requestedControl = CameraMouseControl::Pan;
    }

    const bool mouseControlChanged =
        requestedControl != environment->mouseControl;

    if (mouseControlChanged)
    {
        environment->mouseControl = requestedControl;

        if (requestedControl == CameraMouseControl::None)
        {
            EnableCursor();
        }
        else
        {
            DisableCursor();
        }
    }

    const float movementStep = cameraMoveSpeed * GetFrameTime();
    Vector3 movement = {};

    if (IsKeyDown(KEY_W)) movement.x += movementStep;
    if (IsKeyDown(KEY_S)) movement.x -= movementStep;
    if (IsKeyDown(KEY_D)) movement.y += movementStep;
    if (IsKeyDown(KEY_A)) movement.y -= movementStep;
    if (IsKeyDown(KEY_SPACE)) movement.z += movementStep;
    if (IsKeyDown(KEY_LEFT_CONTROL)) movement.z -= movementStep;

    Vector3 rotation = {};

    if (!mouseControlChanged)
    {
        const Vector2 mouseDelta = GetMouseDelta();

        if (environment->mouseControl == CameraMouseControl::Rotate)
        {
            rotation.x = mouseDelta.x * mouseSensitivity;
            rotation.y = mouseDelta.y * mouseSensitivity;
        }
        else if (environment->mouseControl == CameraMouseControl::Pan)
        {
            movement.y -= mouseDelta.x * mousePanSensitivity;
            movement.z += mouseDelta.y * mousePanSensitivity;
        }
    }

    UpdateCameraPro(
        &environment->camera,
        movement,
        rotation,
        blockMouseInput ? 0.0f : -GetMouseWheelMove());
}

// ----------------------------
// DrawSkyBackground
//
// Desenha o fundo de ceu e sol antes da cena 3D.
//
// ----------------------------
void DrawSkyBackground()
{
    const int screenWidth  = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    ClearBackground(skyTopColor);
    DrawRectangleGradientV(
        0,
        0,
        screenWidth,
        screenHeight,
        skyTopColor,
        skyHorizonColor);
    DrawCircleGradient(
        {
            static_cast<float>(screenWidth) * 0.82f,
            static_cast<float>(screenHeight) * 0.18f
        },
        static_cast<float>(screenHeight) * 0.16f,
        sunCoreColor,
        sunHaloColor);
}

// ----------------------------
// DrawEnvironment3D
//
// Desenha a grade de referencia e os eixos do mundo.
//
// ----------------------------
void DrawEnvironment3D(const Environment3D &environment)
{
    const float extent = static_cast<float>(environment.gridHalfSize) * environment.gridSpacing;

    for (int coordinate = -environment.gridHalfSize; coordinate <= environment.gridHalfSize; ++coordinate)
    {
        if (coordinate == 0)
        {
            continue;
        }

        const float position =
            static_cast<float>(coordinate) * environment.gridSpacing;

        DrawLine3D(
            { position, 0.0f, -extent },
            { position, 0.0f, extent },
            gridColor);
        DrawLine3D(
            { -extent, 0.0f, position },
            { extent, 0.0f, position },
            gridColor);
    }

    DrawLine3D({ -extent, 0.0f, 0.0f }, { extent, 0.0f, 0.0f }, RED);
    DrawLine3D({ 0.0f, 0.0f, -extent }, { 0.0f, 0.0f, extent }, BLUE);
    DrawLine3D({ 0.0f, 0.0f, 0.0f }, { 0.0f, axisHeight, 0.0f }, GREEN);
}

// ----------------------------
// ShutdownEnvironment3D
//
// Restaura o cursor ao encerrar a cena.
//
// ----------------------------
void ShutdownEnvironment3D()
{
    EnableCursor();
}
