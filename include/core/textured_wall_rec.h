#pragma once
#include "wall.h"
#include "draw_utils.h"
#include "managed_texture.h"

// final means that no other class can inherit from this class
class TexturedWallRec final : public Wall {
private:
    const ManagedTexture& m_texture;
    Rectangle m_source;

public:
    TexturedWallRec(Vector3 position, Vector3 size, const ManagedTexture& texture, const Rectangle& source);

    void Draw() const override;
};
