#include "terrain_panel.h"

#include "raylib.h"

#include <algorithm>
#include <cstdlib>

namespace
{
constexpr float sidebarWidth = 302.0f;
constexpr float fieldHeight = 40.0f;
constexpr int maximumCharacters = 3;
constexpr int minimumTerrainSize = 1;
constexpr int maximumTerrainSize = 256;

constexpr Color sidebarBackground = { 244, 246, 248, 255 };
constexpr Color sidebarBorder = { 198, 202, 208, 255 };
constexpr Color headerColor = { 35, 39, 44, 255 };
constexpr Color textColor = { 44, 48, 54, 255 };
constexpr Color mutedTextColor = { 101, 108, 118, 255 };
constexpr Color inputBackground = { 255, 255, 255, 255 };
constexpr Color inputBorder = { 167, 173, 182, 255 };
constexpr Color activeBorder = { 40, 116, 196, 255 };
constexpr Color buttonColor = { 40, 116, 196, 255 };
constexpr Color buttonHoverColor = { 31, 98, 170, 255 };

Rectangle GetSidebarBounds()
{
    return {
        static_cast<float>(GetScreenWidth()) - sidebarWidth,
        0.0f,
        sidebarWidth,
        static_cast<float>(GetScreenHeight())
    };
}

std::array<Rectangle, 2> GetInputBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        Rectangle {
            sidebar.x + 110.0f,
            122.0f,
            sidebar.width - 130.0f,
            fieldHeight
        },
        Rectangle {
            sidebar.x + 110.0f,
            178.0f,
            sidebar.width - 130.0f,
            fieldHeight
        }
    };
}

Rectangle GetApplyButtonBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        sidebar.x + 20.0f,
        242.0f,
        sidebar.width - 40.0f,
        44.0f
    };
}

std::size_t ToIndex(int value)
{
    return static_cast<std::size_t>(value);
}

int ParseTerrainSize(const std::string &value)
{
    if (value.empty())
    {
        return minimumTerrainSize;
    }

    return std::clamp(
        std::atoi(value.c_str()),
        minimumTerrainSize,
        maximumTerrainSize);
}

void ApplyTerrainSize(
    TerrainPanel *panel,
    TerrainPlane *terrain)
{
    terrain->widthCells = ParseTerrainSize(panel->values[0]);
    terrain->heightCells = ParseTerrainSize(panel->values[1]);
    terrain->created = true;
    panel->values[0] = std::to_string(terrain->widthCells);
    panel->values[1] = std::to_string(terrain->heightCells);
    panel->activeField = -1;
    panel->replaceOnNextInput = false;
}

void UpdateActiveInput(TerrainPanel *panel)
{
    if (panel->activeField < 0)
    {
        return;
    }

    std::string &value =
        panel->values[ToIndex(panel->activeField)];
    int codepoint = GetCharPressed();

    while (codepoint > 0)
    {
        if (codepoint >= '0' && codepoint <= '9')
        {
            if (panel->replaceOnNextInput)
            {
                value.clear();
                panel->replaceOnNextInput = false;
            }

            if (static_cast<int>(value.size()) < maximumCharacters)
            {
                value.push_back(static_cast<char>(codepoint));
            }
        }

        codepoint = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE))
    {
        if (panel->replaceOnNextInput)
        {
            value.clear();
            panel->replaceOnNextInput = false;
        }
        else if (!value.empty())
        {
            value.pop_back();
        }
    }

    if (IsKeyPressed(KEY_TAB))
    {
        panel->activeField = (panel->activeField + 1) % 2;
        panel->replaceOnNextInput = true;
    }
    else if (IsKeyPressed(KEY_ESCAPE))
    {
        panel->activeField = -1;
        panel->replaceOnNextInput = false;
    }
}

void DrawInput(
    const TerrainPanel &panel,
    int field,
    Rectangle bounds,
    const char *label)
{
    const bool active = panel.activeField == field;

    DrawText(
        label,
        static_cast<int>(bounds.x - 90.0f),
        static_cast<int>(bounds.y + 10.0f),
        18,
        textColor);
    DrawRectangleRec(bounds, inputBackground);
    DrawRectangleLinesEx(
        bounds,
        active ? 2.0f : 1.0f,
        active ? activeBorder : inputBorder);
    DrawText(
        panel.values[ToIndex(field)].c_str(),
        static_cast<int>(bounds.x + 12.0f),
        static_cast<int>(bounds.y + 10.0f),
        20,
        textColor);

    if (active && (static_cast<int>(GetTime() * 2.0) % 2 == 0))
    {
        const int textWidth =
            MeasureText(panel.values[ToIndex(field)].c_str(), 20);
        const int cursorX =
            static_cast<int>(bounds.x + 13.0f) + textWidth;
        DrawLine(
            cursorX,
            static_cast<int>(bounds.y + 8.0f),
            cursorX,
            static_cast<int>(bounds.y + bounds.height - 8.0f),
            textColor);
    }
}
}

TerrainPanel CreateTerrainPanel()
{
    TerrainPanel panel = {};
    panel.values = { "8", "8" };
    panel.activeField = -1;
    panel.replaceOnNextInput = false;
    return panel;
}

bool UpdateTerrainPanel(
    TerrainPanel *panel,
    TerrainPlane *terrain)
{
    const Rectangle sidebar = GetSidebarBounds();
    const std::array<Rectangle, 2> inputs = GetInputBounds();
    const Rectangle button = GetApplyButtonBounds();
    const Vector2 mousePosition = GetMousePosition();
    const bool mouseInside =
        CheckCollisionPointRec(mousePosition, sidebar);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        int clickedField = -1;

        for (int field = 0; field < 2; ++field)
        {
            if (CheckCollisionPointRec(
                    mousePosition,
                    inputs[ToIndex(field)]))
            {
                clickedField = field;
                break;
            }
        }

        if (clickedField >= 0)
        {
            panel->activeField = clickedField;
            panel->replaceOnNextInput = true;
        }
        else if (CheckCollisionPointRec(mousePosition, button))
        {
            ApplyTerrainSize(panel, terrain);
        }
        else if (!mouseInside)
        {
            panel->activeField = -1;
            panel->replaceOnNextInput = false;
        }
    }

    UpdateActiveInput(panel);

    if (panel->activeField >= 0 && IsKeyPressed(KEY_ENTER))
    {
        ApplyTerrainSize(panel, terrain);
    }

    return mouseInside || panel->activeField >= 0;
}

void DrawTerrainPanel(
    const TerrainPanel &panel,
    const TerrainPlane &terrain)
{
    const Rectangle sidebar = GetSidebarBounds();
    const std::array<Rectangle, 2> inputs = GetInputBounds();
    const Rectangle button = GetApplyButtonBounds();
    const bool buttonHovered =
        CheckCollisionPointRec(GetMousePosition(), button);

    DrawRectangleRec(sidebar, sidebarBackground);
    DrawLine(
        static_cast<int>(sidebar.x),
        0,
        static_cast<int>(sidebar.x),
        GetScreenHeight(),
        sidebarBorder);
    DrawText(
        "Terreno",
        static_cast<int>(sidebar.x + 20.0f),
        24,
        26,
        headerColor);
    DrawText(
        "Dimensoes em celulas",
        static_cast<int>(sidebar.x + 20.0f),
        60,
        16,
        mutedTextColor);

    DrawInput(panel, 0, inputs[0], "Largura");
    DrawInput(panel, 1, inputs[1], "Altura");

    DrawRectangleRec(
        button,
        buttonHovered ? buttonHoverColor : buttonColor);
    DrawText(
        "Aplicar tamanho",
        static_cast<int>(
            button.x +
            (button.width - static_cast<float>(
                MeasureText("Aplicar tamanho", 19))) * 0.5f),
        static_cast<int>(button.y + 12.0f),
        19,
        RAYWHITE);

    DrawText(
        TextFormat(
            "%i x %i celulas",
            terrain.widthCells,
            terrain.heightCells),
        static_cast<int>(sidebar.x + 20.0f),
        318,
        17,
        textColor);
    DrawText(
        TextFormat("Celula %.1fm", static_cast<double>(terrain.cellSize)),
        static_cast<int>(sidebar.x + 20.0f),
        344,
        16,
        mutedTextColor);
}
