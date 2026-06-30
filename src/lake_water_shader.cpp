#include "lake_water_shader.h"

namespace
{
constexpr Color shallowWaterColor   = { 130, 183, 194, 136 };
constexpr Color deepWaterColor      = { 66, 135, 174, 150 };
constexpr Color highlightWaterColor = { 238, 247, 238, 230 };

// ----------------------------
// GetLakeWaterShaderPath
//
// Escolhe o caminho do fragment shader da agua a partir do diretorio atual.
//
// ----------------------------
const char *GetLakeWaterShaderPath()
{
    if (FileExists("shaders/lake_water.fs"))
    {
        return "shaders/lake_water.fs";
    }

    return "../../shaders/lake_water.fs";
}

// ----------------------------
// ColorToUniform
//
// Converte Color para vec4 normalizado usado pelo shader.
//
// ----------------------------
Vector4 ColorToUniform(Color color)
{
    return {
        static_cast<float>(color.r) / 255.0f,
        static_cast<float>(color.g) / 255.0f,
        static_cast<float>(color.b) / 255.0f,
        static_cast<float>(color.a) / 255.0f
    };
}
}

// ----------------------------
// CreateLakeWaterShader
//
// Carrega o shader usado para desenhar lagos estilizados.
//
// ----------------------------
LakeWaterShader CreateLakeWaterShader()
{
    LakeWaterShader water = {};
    water.shader                 = LoadShader(nullptr, GetLakeWaterShaderPath());
    water.timeLocation           = GetShaderLocation(water.shader, "time");
    water.shallowColorLocation   =
        GetShaderLocation(water.shader, "shallowColor");
    water.deepColorLocation      =
        GetShaderLocation(water.shader, "deepColor");
    water.highlightColorLocation =
        GetShaderLocation(water.shader, "highlightColor");
    water.edgeMaskLocation       =
        GetShaderLocation(water.shader, "edgeMask");
    water.ready                  = IsShaderValid(water.shader);
    return water;
}

// ----------------------------
// UpdateLakeWaterShader
//
// Atualiza tempo e paleta do shader da agua.
//
// ----------------------------
void UpdateLakeWaterShader(
    LakeWaterShader *water,
    float time)
{
    if (!water->ready)
    {
        return;
    }

    const Vector4 shallowColor   = ColorToUniform(shallowWaterColor);
    const Vector4 deepColor      = ColorToUniform(deepWaterColor);
    const Vector4 highlightColor = ColorToUniform(highlightWaterColor);

    SetShaderValue(
        water->shader,
        water->timeLocation,
        &time,
        SHADER_UNIFORM_FLOAT);
    SetShaderValue(
        water->shader,
        water->shallowColorLocation,
        &shallowColor,
        SHADER_UNIFORM_VEC4);
    SetShaderValue(
        water->shader,
        water->deepColorLocation,
        &deepColor,
        SHADER_UNIFORM_VEC4);
    SetShaderValue(
        water->shader,
        water->highlightColorLocation,
        &highlightColor,
        SHADER_UNIFORM_VEC4);
}

// ----------------------------
// BeginLakeWaterShader
//
// Inicia o shader da agua com mascara de borda do tile.
//
// ----------------------------
void BeginLakeWaterShader(
    const LakeWaterShader *water,
    Vector4 edgeMask)
{
    if (water != nullptr && water->ready)
    {
        SetShaderValue(
            water->shader,
            water->edgeMaskLocation,
            &edgeMask,
            SHADER_UNIFORM_VEC4);
        BeginShaderMode(water->shader);
    }
}

// ----------------------------
// EndLakeWaterShader
//
// Finaliza o shader da agua quando ele esta ativo.
//
// ----------------------------
void EndLakeWaterShader(const LakeWaterShader *water)
{
    if (water != nullptr && water->ready)
    {
        EndShaderMode();
    }
}

// ----------------------------
// ShutdownLakeWaterShader
//
// Libera o shader da agua da GPU.
//
// ----------------------------
void ShutdownLakeWaterShader(LakeWaterShader *water)
{
    if (water->ready)
    {
        UnloadShader(water->shader);
    }

    water->ready = false;
}
