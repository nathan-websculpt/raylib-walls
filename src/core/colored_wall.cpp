#include "../../include/core/colored_wall.h"

ColoredWall::ColoredWall(Vector3 position, Vector3 size, Color color)
    : Wall(position, size), m_color(color) {}

void ColoredWall::Draw() const {
    DrawCube(GetPosition(), GetSize().x, GetSize().y, GetSize().z, m_color);
}
