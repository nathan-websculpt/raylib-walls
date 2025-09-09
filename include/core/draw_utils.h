#pragma once
#include "raylib.h"
#include "rlgl.h"

// Draw cube textured on all faces
void DrawCubeTexture(const Texture2D& texture, const Vector3& position,
                     float width, float height, float length, Color color);

// Draw cube with a texture sub-rectangle applied to all faces
void DrawCubeTextureRec(const Texture2D& texture, const Rectangle& source, const Vector3& position,
                        float width, float height, float length, Color color);
