#include "../../include/core/wall_handler.h"

void WallHandler::AddWall(std::unique_ptr<Wall> wall) {
    m_walls.emplace_back(std::move(wall)); // emplace_back constructs the object in-place, directly in the vector's memory
}

void WallHandler::DrawWalls(bool debug) const {
    for (const std::unique_ptr<Wall>& wall : m_walls) {
        wall->Draw();
        if (debug) wall->DrawDebug();
    }
}
