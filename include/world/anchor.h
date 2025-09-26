#pragma once
#include "../ecs/components.h"
#include "../ecs/registry.h"
#include "doorway.h"
#include "../ecs/entity_utils.h"

inline Wall::Side AnchorToWallSide(Vector3 dir) {
    if (dir.z < -0.9f) return Wall::Side::Front;
    if (dir.z > 0.9f)  return Wall::Side::Back;
    if (dir.x < -0.9f) return Wall::Side::Left;
    if (dir.x > 0.9f)  return Wall::Side::Right;
    return Wall::Side::Front;
}

inline void CarveDoorwayInWall(Registry& reg, Entity room, Wall::Side side) {
    if (auto children = reg.get<Children>(room)) {
        // make a copy to avoid modifying while iterating
        auto childEntities = children->entities;
        
        for (Entity child : childEntities) {
            if (auto wall = reg.get<Wall>(child)) {
                if (wall->side == side) {
                    if (auto t = reg.get<TransformComp>(child)) {
                        Vector3 pos = t->position;
                        Vector3 size = t->size;
                        
                        std::shared_ptr<ManagedTexture> tex = nullptr;
                        if (auto tr = reg.get<TexturedRender>(child)) {
                            tex = tr->texture;
                        }
                        
                        // destroy the wall and create a doorway
                        DestroyEntityWithChildren(reg, child);
                        MakeWallWithDoor(reg, room, pos, size, tex, true);
                        break; // only carve one doorway per side
                    }
                }
            }
        }
    }
}

inline void ConnectAnchors(Registry& reg, Entity roomAnchor, Entity hallAnchor) {
    auto ra = reg.get<Anchor>(roomAnchor);
    auto ha = reg.get<Anchor>(hallAnchor);
    if (!ra || !ha) return;
    
    auto roomParentComp = reg.get<Parent>(roomAnchor);
    auto hallParentComp = reg.get<Parent>(hallAnchor);
    if (!roomParentComp || !hallParentComp) return;
    
    Entity room = roomParentComp->parent;
    Entity hall = hallParentComp->parent;
    
    auto rt = reg.get<WorldTransform>(roomAnchor);
    auto ht = reg.get<WorldTransform>(hallAnchor);
    auto hallT = reg.get<TransformComp>(hall);
    
    if (!rt || !ht || !hallT) return;
    
    Vector3 delta = Vector3Subtract(rt->position, ht->position);
    hallT->position = Vector3Add(hallT->position, delta);
    
    // carve doorway in room
    Wall::Side roomSide = AnchorToWallSide(ra->direction);
    CarveDoorwayInWall(reg, room, roomSide);
    
    // carve doorway in hallway
    Wall::Side hallSide = AnchorToWallSide(ha->direction);
    CarveDoorwayInWall(reg, hall, hallSide);
    
    ra->connectedTo = hallAnchor;
    ha->connectedTo = roomAnchor;
}