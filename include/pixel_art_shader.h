#ifndef PIXEL_ART_SHADER_H
#define PIXEL_ART_SHADER_H

#include "raylib.h"

struct PixelArtShader
{
    Shader shader;
    RenderTexture2D target;
    int resolutionLocation;
    int pixelSizeLocation;
    float pixelSize;
    bool ready;
};

PixelArtShader CreatePixelArtShader(
    int width,
    int height);
void ResizePixelArtShader(
    PixelArtShader *effect,
    int width,
    int height);
void BeginPixelArtScene(const PixelArtShader &effect);
void EndPixelArtScene();
void DrawPixelArtScene(
    PixelArtShader *effect,
    bool enabled);
void ShutdownPixelArtShader(PixelArtShader *effect);

#endif
