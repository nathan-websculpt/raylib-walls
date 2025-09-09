// Wall is an abstract base class (pure virtual Draw()) for all wall types.

#pragma once
#include "raylib.h"

class Wall {
private:
    // prevents accidental modification and allows validation
    Vector3 m_position;
    Vector3 m_size;
    BoundingBox m_boundingBox;

public:
    Wall(Vector3 position, Vector3 size);
    virtual ~Wall() = default;

    virtual void Draw() const = 0;     // Pure virtual
    virtual void DrawDebug() const;

    [[nodiscard]] BoundingBox GetBoundingBox() const { return m_boundingBox; }

    // getters for private members
    [[nodiscard]] Vector3 GetPosition() const { return m_position; }
    [[nodiscard]] Vector3 GetSize() const { return m_size; }
};
