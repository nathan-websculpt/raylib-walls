#include "../../include/core/textured_wall.h"

TexturedWall::TexturedWall(Vector3 position, Vector3 size, const ManagedTexture& texture)
    : Wall(position, size), m_texture(texture) {}

void TexturedWall::Draw() const {
    DrawCubeTexture(m_texture.get(), GetPosition(), GetSize().x, GetSize().y, GetSize().z, WHITE);
}
