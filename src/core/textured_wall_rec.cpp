#include "../../include/core/textured_wall_rec.h"

TexturedWallRec::TexturedWallRec(Vector3 position, Vector3 size, const ManagedTexture& texture, const Rectangle& source)
    : Wall(position, size), m_texture(texture), m_source(source) {}

void TexturedWallRec::Draw() const {
    DrawCubeTextureRec(m_texture.get(), m_source, GetPosition(), GetSize().x, GetSize().y, GetSize().z, WHITE);
}
