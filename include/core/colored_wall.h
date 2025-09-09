#pragma once
#include "wall.h"

// final means that no other class can inherit from this class
class ColoredWall final: public Wall {
private:
    Color m_color;
    
public:
    ColoredWall(Vector3 position, Vector3 size, Color color);
    void Draw() const override;
};
