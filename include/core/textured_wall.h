#pragma once
#include "wall.h"
#include "draw_utils.h"
#include "managed_texture.h"

// final means that no other class can inherit from this class
class TexturedWall final : public Wall {
public:
    TexturedWall(Vector3 position, Vector3 size, const ManagedTexture& texture);

    void Draw() const override;

private:
    const ManagedTexture& m_texture; // Reference to RAII texture
};
