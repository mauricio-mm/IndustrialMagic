#include "terrain/terrain_panel.h"

#include "raylib.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
constexpr float sidebarWidth = 302.0f;
constexpr float fieldHeight = 40.0f;
constexpr int maximumCharacters = 5;
constexpr int minimumTerrainSize = 1;
constexpr int maximumTerrainSize = 256;
constexpr float minimumMountainHeight = 1.0f;
constexpr float maximumMountainHeight = 80.0f;
constexpr float minimumLakeDepth = 0.0f;
constexpr float maximumLakeDepth = 30.0f;

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

// ----------------------------
// GetSidebarBounds
//
// Calcula a area ocupada pela barra lateral do terrain.
//
// ----------------------------
Rectangle GetSidebarBounds()
{
    return {
        static_cast<float>(GetScreenWidth()) - sidebarWidth,
        0.0f,
        sidebarWidth,
        static_cast<float>(GetScreenHeight())
    };
}

// ----------------------------
// GetInputBounds
//
// Retorna as areas dos quatro campos de entrada do painel.
//
// ----------------------------
std::array<Rectangle, 4> GetInputBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        Rectangle {
            sidebar.x + 110.0f,
            108.0f,
            sidebar.width - 130.0f,
            fieldHeight
        },
        Rectangle {
            sidebar.x + 110.0f,
            156.0f,
            sidebar.width - 130.0f,
            fieldHeight
        },
        Rectangle {
            sidebar.x + 110.0f,
            204.0f,
            sidebar.width - 130.0f,
            fieldHeight
        },
        Rectangle {
            sidebar.x + 110.0f,
            252.0f,
            sidebar.width - 130.0f,
            fieldHeight
        }
    };
}

// ----------------------------
// GetApplyButtonBounds
//
// Calcula a area do botao que aplica os valores digitados.
//
// ----------------------------
Rectangle GetApplyButtonBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        sidebar.x + 20.0f,
        308.0f,
        sidebar.width - 40.0f,
        44.0f
    };
}

// ----------------------------
// GetGenerateButtonBounds
//
// Calcula a area do botao que muda a seed do terreno.
//
// ----------------------------
Rectangle GetGenerateButtonBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        sidebar.x + 20.0f,
        362.0f,
        sidebar.width - 40.0f,
        44.0f
    };
}

// ----------------------------
// GetFlatMapButtonBounds
//
// Calcula a area do botao do mapa plano.
//
// ----------------------------
Rectangle GetFlatMapButtonBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        sidebar.x + 20.0f,
        434.0f,
        sidebar.width - 40.0f,
        40.0f
    };
}

// ----------------------------
// GetWorldMapButtonBounds
//
// Calcula a area do botao do mundo quadrado.
//
// ----------------------------
Rectangle GetWorldMapButtonBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        sidebar.x + 20.0f,
        482.0f,
        sidebar.width - 40.0f,
        40.0f
    };
}

// ----------------------------
// GetPixelShaderButtonBounds
//
// Calcula a area do botao que liga o shader pixel art.
//
// ----------------------------
Rectangle GetPixelShaderButtonBounds()
{
    const Rectangle sidebar = GetSidebarBounds();
    return {
        sidebar.x + 20.0f,
        542.0f,
        sidebar.width - 40.0f,
        40.0f
    };
}

// ----------------------------
// ToIndex
//
// Converte um indice inteiro para o tipo usado por arrays.
//
// ----------------------------
std::size_t ToIndex(int value)
{
    return static_cast<std::size_t>(value);
}

// ----------------------------
// ParseTerrainSize
//
// Converte texto para tamanho de terreno dentro dos limites aceitos.
//
// ----------------------------
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

// ----------------------------
// ParseTerrainMeasure
//
// Converte texto para medida em metros dentro dos limites aceitos.
//
// ----------------------------
float ParseTerrainMeasure(
    const std::string &value,
    float minimumValue,
    float maximumValue)
{
    if (value.empty())
    {
        return minimumValue;
    }

    return std::clamp(
        std::strtof(value.c_str(), nullptr),
        minimumValue,
        maximumValue);
}

// ----------------------------
// FormatMeasure
//
// Formata medidas sem casas decimais quando o valor e inteiro.
//
// ----------------------------
std::string FormatMeasure(float value)
{
    if (std::fabs(value - std::round(value)) < 0.001f)
    {
        return std::to_string(static_cast<int>(std::round(value)));
    }

    char buffer[16] = {};
    std::snprintf(buffer, sizeof(buffer), "%.1f", static_cast<double>(value));
    return buffer;
}

// ----------------------------
// ApplyTerrainSize
//
// Aplica largura, altura, montanhas e lagos digitados no painel.
//
// ----------------------------
void ApplyTerrainSize(
    TerrainPanel *panel,
    TerrainPlane *terrain)
{
    terrain->widthCells    = ParseTerrainSize(panel->values[0]);
    terrain->heightCells   = ParseTerrainSize(panel->values[1]);
    terrain->maximumHeight = ParseTerrainMeasure(
        panel->values[2],
        minimumMountainHeight,
        maximumMountainHeight);
    terrain->lakeDepth = ParseTerrainMeasure(
        panel->values[3],
        minimumLakeDepth,
        maximumLakeDepth);
    GenerateTerrainPlane(terrain);
    panel->values[0]          = std::to_string(terrain->widthCells);
    panel->values[1]          = std::to_string(terrain->heightCells);
    panel->values[2]          = FormatMeasure(terrain->maximumHeight);
    panel->values[3]          = FormatMeasure(terrain->lakeDepth);
    panel->activeField        = -1;
    panel->replaceOnNextInput = false;
}

// ----------------------------
// GenerateNewTerrain
//
// Incrementa a seed e regenera o terreno com os valores atuais.
//
// ----------------------------
void GenerateNewTerrain(
    TerrainPanel *panel,
    TerrainPlane *terrain)
{
    terrain->seed += 1u;
    ApplyTerrainSize(panel, terrain);
}

// ----------------------------
// PrepareForReplacement
//
// Limpa o campo quando o proximo input deve substituir o valor atual.
//
// ----------------------------
void PrepareForReplacement(TerrainPanel *panel, std::string *value)
{
    if (panel->replaceOnNextInput)
    {
        value->clear();
        panel->replaceOnNextInput = false;
    }
}

// ----------------------------
// CanAppendCharacter
//
// Verifica se o campo ainda aceita mais caracteres.
//
// ----------------------------
bool CanAppendCharacter(const std::string &value)
{
    return static_cast<int>(value.size()) < maximumCharacters;
}

// ----------------------------
// UpdateActiveInput
//
// Processa teclado, backspace e escape no campo ativo.
//
// ----------------------------
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
            PrepareForReplacement(panel, &value);

            if (CanAppendCharacter(value))
            {
                value.push_back(static_cast<char>(codepoint));
            }
        }
        else if (
            panel->activeField >= 2 &&
            (codepoint == '.' || codepoint == ',') &&
            CanAppendCharacter(value))
        {
            PrepareForReplacement(panel, &value);

            if (value.find('.') == std::string::npos)
            {
                value.push_back('.');
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

    if (IsKeyPressed(KEY_ESCAPE))
    {
        panel->activeField = -1;
        panel->replaceOnNextInput = false;
    }
}

// ----------------------------
// DrawInput
//
// Desenha um campo de texto com label, borda ativa e cursor.
//
// ----------------------------
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
        const int cursorX   =
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

// ----------------------------
// CreateTerrainPanel
//
// Cria o painel com valores iniciais para o terreno.
//
// ----------------------------
TerrainPanel CreateTerrainPanel()
{
    TerrainPanel panel = {};
    panel.values             = { "32", "32", "15", "4" };
    panel.activeField        = -1;
    panel.replaceOnNextInput = false;
    panel.tabPressed         = false;
    panel.pixelShaderEnabled = false;
    return panel;
}

// ----------------------------
// UpdateTerrainPanel
//
// Atualiza inputs, botoes e captura de mouse do painel.
//
// ----------------------------
bool UpdateTerrainPanel(
    TerrainPanel *panel,
    TerrainPlane *terrain)
{
    const Rectangle sidebar                       = GetSidebarBounds();
    const std::array<Rectangle, 4> inputs         = GetInputBounds();
    const Rectangle button                        = GetApplyButtonBounds();
    const Rectangle generateButton                = GetGenerateButtonBounds();
    const Rectangle flatMapButton                 = GetFlatMapButtonBounds();
    const Rectangle worldMapButton                = GetWorldMapButtonBounds();
    const Rectangle pixelShaderButton             = GetPixelShaderButtonBounds();
    const Vector2 mousePosition                   = GetMousePosition();
    const bool mouseInside                        =
        CheckCollisionPointRec(mousePosition, sidebar);
    panel->tabPressed = IsKeyPressed(KEY_TAB);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        int clickedField = -1;

        for (int field = 0; field < 4; ++field)
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
        else if (CheckCollisionPointRec(mousePosition, generateButton))
        {
            GenerateNewTerrain(panel, terrain);
        }
        else if (CheckCollisionPointRec(mousePosition, flatMapButton))
        {
            SetTerrainMapKind(terrain, TerrainMapKind::FlatGrid);
        }
        else if (CheckCollisionPointRec(mousePosition, worldMapButton))
        {
            SetTerrainMapKind(terrain, TerrainMapKind::LowPolyWorld);
        }
        else if (CheckCollisionPointRec(mousePosition, pixelShaderButton))
        {
            panel->pixelShaderEnabled = !panel->pixelShaderEnabled;
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

// ----------------------------
// DrawTerrainPanel
//
// Desenha a barra lateral com campos, botoes e status do terreno.
//
// ----------------------------
void DrawTerrainPanel(
    const TerrainPanel &panel,
    const TerrainPlane &terrain)
{
    const Rectangle sidebar                       = GetSidebarBounds();
    const std::array<Rectangle, 4> inputs         = GetInputBounds();
    const Rectangle button                        = GetApplyButtonBounds();
    const Rectangle generateButton                = GetGenerateButtonBounds();
    const Rectangle flatMapButton                 = GetFlatMapButtonBounds();
    const Rectangle worldMapButton                = GetWorldMapButtonBounds();
    const Rectangle pixelShaderButton             = GetPixelShaderButtonBounds();
    const bool buttonHovered                      =
        CheckCollisionPointRec(GetMousePosition(), button);
    const bool generateButtonHovered              =
        CheckCollisionPointRec(GetMousePosition(), generateButton);
    const bool flatMapHovered                     =
        CheckCollisionPointRec(GetMousePosition(), flatMapButton);
    const bool worldMapHovered                    =
        CheckCollisionPointRec(GetMousePosition(), worldMapButton);
    const bool pixelShaderHovered                 =
        CheckCollisionPointRec(GetMousePosition(), pixelShaderButton);
    const char *pixelShaderLabel                  =
        panel.pixelShaderEnabled ? "Pixel art ligado" : "Pixel art desligado";

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
    DrawInput(panel, 2, inputs[2], "Monte m");
    DrawInput(panel, 3, inputs[3], "Lago m");

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

    DrawRectangleRec(
        generateButton,
        generateButtonHovered ? buttonHoverColor : buttonColor);
    DrawText(
        "Gerar novo",
        static_cast<int>(
            generateButton.x +
            (generateButton.width - static_cast<float>(
                MeasureText("Gerar novo", 19))) * 0.5f),
        static_cast<int>(generateButton.y + 12.0f),
        19,
        RAYWHITE);

    DrawText(
        "Mapa",
        static_cast<int>(sidebar.x + 20.0f),
        416,
        17,
        textColor);
    DrawRectangleRec(
        flatMapButton,
        terrain.mapKind == TerrainMapKind::FlatGrid
            ? activeBorder
            : (flatMapHovered ? buttonHoverColor : buttonColor));
    DrawText(
        "Plano",
        static_cast<int>(
            flatMapButton.x +
            (flatMapButton.width - static_cast<float>(
                MeasureText("Plano", 18))) * 0.5f),
        static_cast<int>(flatMapButton.y + 11.0f),
        18,
        RAYWHITE);
    DrawRectangleRec(
        worldMapButton,
        terrain.mapKind == TerrainMapKind::LowPolyWorld
            ? activeBorder
            : (worldMapHovered ? buttonHoverColor : buttonColor));
    DrawText(
        "Mundo quadrado",
        static_cast<int>(
            worldMapButton.x +
            (worldMapButton.width - static_cast<float>(
                MeasureText("Mundo quadrado", 18))) * 0.5f),
        static_cast<int>(worldMapButton.y + 11.0f),
        18,
        RAYWHITE);

    DrawText(
        "Visual",
        static_cast<int>(sidebar.x + 20.0f),
        524,
        17,
        textColor);
    DrawRectangleRec(
        pixelShaderButton,
        panel.pixelShaderEnabled
            ? activeBorder
            : (pixelShaderHovered ? buttonHoverColor : buttonColor));
    DrawText(
        pixelShaderLabel,
        static_cast<int>(
            pixelShaderButton.x +
            (pixelShaderButton.width - static_cast<float>(
                MeasureText(pixelShaderLabel, 18))) * 0.5f),
        static_cast<int>(pixelShaderButton.y + 11.0f),
        18,
        RAYWHITE);

    DrawText(
        GetTerrainMapName(terrain),
        static_cast<int>(sidebar.x + 20.0f),
        604,
        17,
        textColor);
    DrawText(
        TextFormat(
            "Montanhas %.1fm  Lagos %.1fm",
            static_cast<double>(terrain.maximumHeight),
            static_cast<double>(terrain.lakeDepth)),
        static_cast<int>(sidebar.x + 20.0f),
        630,
        16,
        mutedTextColor);
    DrawText(
        TextFormat(
            "Malha %s  TAB",
            terrain.showMesh ? "ligada" : "desligada"),
        static_cast<int>(sidebar.x + 20.0f),
        654,
        16,
        mutedTextColor);
    DrawText(
        TextFormat(
            "Noise %.2fx  segure R + scroll",
            static_cast<double>(terrain.noiseScale)),
        static_cast<int>(sidebar.x + 20.0f),
        678,
        16,
        mutedTextColor);
    DrawText(
        TextFormat(
            "%i x %i celulas  Seed %u",
            terrain.widthCells,
            terrain.heightCells,
            terrain.seed),
        static_cast<int>(sidebar.x + 20.0f),
        702,
        16,
        mutedTextColor);
}
