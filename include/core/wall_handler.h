#pragma once
#include <memory>
#include <vector>
#include "wall.h"

class WallHandler {
private:
    std::vector<std::unique_ptr<Wall>> m_walls;

public:
    WallHandler() = default;

    // make intent of class clearer to other developers...
    // explicitly deleting the copy constructor, class is now non-copyable but movable, matching the semantics of its member variables
    // because std::unique_ptr is a move-only type
    WallHandler(const WallHandler&) = delete;
    // deleting the copy assignment operator prevents the class from being copied via assignment
    WallHandler& operator=(const WallHandler&) = delete;
    // tell the compiler to generate the default move behavior, which correctly moves the contents of the std::vector from one WallHandler to another
    WallHandler(WallHandler&&) noexcept = default;
    // declaring the move assignment operator with = default ensures that the class is efficiently movable via assignment
    WallHandler& operator=(WallHandler&&) noexcept = default;

    void AddWall(std::unique_ptr<Wall> wall);
    void DrawWalls(bool debug = false) const;
};
