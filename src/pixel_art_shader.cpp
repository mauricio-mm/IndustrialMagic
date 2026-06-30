#include "pixel_art_shader.h"

namespace
{
constexpr float defaultPixelSize = 5.0f;

// ----------------------------
// GetPixelArtShaderPath
//
// Escolhe o caminho do fragment shader a partir do diretorio atual.
//
// ----------------------------
const char *GetPixelArtShaderPath()
{
    if (FileExists("shaders/pixel_art.fs"))
    {
        return "shaders/pixel_art.fs";
    }

    return "../../shaders/pixel_art.fs";
}

// ----------------------------
// LoadPixelArtTarget
//
// Cria a textura de render usada pelo pos-processamento.
//
// ----------------------------
void LoadPixelArtTarget(
    PixelArtShader *effect,
    int width,
    int height)
{
    effect->target = LoadRenderTexture(width, height);

    if (IsRenderTextureValid(effect->target))
    {
        SetTextureFilter(effect->target.texture, TEXTURE_FILTER_POINT);
    }
}

// ----------------------------
// UpdatePixelArtUniforms
//
// Envia resolucao e tamanho do pixel para o shader.
//
// ----------------------------
void UpdatePixelArtUniforms(PixelArtShader *effect)
{
    const Vector2 resolution = {
        static_cast<float>(GetScreenWidth()),
        static_cast<float>(GetScreenHeight())
    };

    SetShaderValue(
        effect->shader,
        effect->resolutionLocation,
        &resolution,
        SHADER_UNIFORM_VEC2);
    SetShaderValue(
        effect->shader,
        effect->pixelSizeLocation,
        &effect->pixelSize,
        SHADER_UNIFORM_FLOAT);
}

// ----------------------------
// GetRenderSource
//
// Retorna a area da textura invertida no eixo Y para desenhar na tela.
//
// ----------------------------
Rectangle GetRenderSource(const PixelArtShader &effect)
{
    return {
        0.0f,
        0.0f,
        static_cast<float>(effect.target.texture.width),
        -static_cast<float>(effect.target.texture.height)
    };
}

// ----------------------------
// GetRenderDestination
//
// Retorna a area da tela ocupada pela textura renderizada.
//
// ----------------------------
Rectangle GetRenderDestination()
{
    return {
        0.0f,
        0.0f,
        static_cast<float>(GetScreenWidth()),
        static_cast<float>(GetScreenHeight())
    };
}
}

// ----------------------------
// CreatePixelArtShader
//
// Carrega o shader e cria o alvo de render para aplicar pixel art.
//
// ----------------------------
PixelArtShader CreatePixelArtShader(
    int width,
    int height)
{
    PixelArtShader effect = {};
    effect.shader             = LoadShader(nullptr, GetPixelArtShaderPath());
    effect.resolutionLocation = GetShaderLocation(effect.shader, "resolution");
    effect.pixelSizeLocation  = GetShaderLocation(effect.shader, "pixelSize");
    effect.pixelSize          = defaultPixelSize;

    LoadPixelArtTarget(&effect, width, height);
    effect.ready = IsShaderValid(effect.shader) &&
        IsRenderTextureValid(effect.target);
    return effect;
}

// ----------------------------
// ResizePixelArtShader
//
// Recria a textura de render quando a janela muda de tamanho.
//
// ----------------------------
void ResizePixelArtShader(
    PixelArtShader *effect,
    int width,
    int height)
{
    if (
        !IsRenderTextureValid(effect->target) ||
        effect->target.texture.width != width ||
        effect->target.texture.height != height)
    {
        if (IsRenderTextureValid(effect->target))
        {
            UnloadRenderTexture(effect->target);
        }

        LoadPixelArtTarget(effect, width, height);
        effect->ready = IsShaderValid(effect->shader) &&
            IsRenderTextureValid(effect->target);
    }
}

// ----------------------------
// BeginPixelArtScene
//
// Inicia o desenho da cena na textura usada pelo shader.
//
// ----------------------------
void BeginPixelArtScene(const PixelArtShader &effect)
{
    BeginTextureMode(effect.target);
}

// ----------------------------
// EndPixelArtScene
//
// Finaliza o desenho da cena na textura usada pelo shader.
//
// ----------------------------
void EndPixelArtScene()
{
    EndTextureMode();
}

// ----------------------------
// DrawPixelArtScene
//
// Desenha a textura da cena com ou sem o shader pixel art.
//
// ----------------------------
void DrawPixelArtScene(
    PixelArtShader *effect,
    bool enabled)
{
    const Rectangle source      = GetRenderSource(*effect);
    const Rectangle destination = GetRenderDestination();

    if (enabled && effect->ready)
    {
        UpdatePixelArtUniforms(effect);
        BeginShaderMode(effect->shader);
        DrawTexturePro(
            effect->target.texture,
            source,
            destination,
            { 0.0f, 0.0f },
            0.0f,
            WHITE);
        EndShaderMode();
        return;
    }

    DrawTexturePro(
        effect->target.texture,
        source,
        destination,
        { 0.0f, 0.0f },
        0.0f,
        WHITE);
}

// ----------------------------
// ShutdownPixelArtShader
//
// Libera shader e textura usados pelo pos-processamento.
//
// ----------------------------
void ShutdownPixelArtShader(PixelArtShader *effect)
{
    if (IsRenderTextureValid(effect->target))
    {
        UnloadRenderTexture(effect->target);
    }

    if (IsShaderValid(effect->shader))
    {
        UnloadShader(effect->shader);
    }

    effect->ready = false;
}
