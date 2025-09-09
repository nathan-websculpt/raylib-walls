#include "../../include/core/wall.h"

Wall::Wall(Vector3 position, Vector3 size)
    : m_position(position), m_size(size)
{
    Vector3 min = { position.x - size.x/2, position.y - size.y/2, position.z - size.z/2 };
    Vector3 max = { position.x + size.x/2, position.y + size.y/2, position.z + size.z/2 };
    m_boundingBox = { min, max };
}

void Wall::DrawDebug() const {
    DrawBoundingBox(m_boundingBox, RED);
}
