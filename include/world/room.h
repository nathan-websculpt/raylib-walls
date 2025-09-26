#pragma once
#include "../ecs/components.h"
#include "../ecs/registry.h"
#include "../textures/managed_texture.h"
#include <algorithm>
#include <vector>
#include <iostream>
#include <memory>

// room with optional skipped walls (skipWalls are currently full openings)
inline Entity CreateRoom(Registry& reg, Vector3 pos, Vector3 size, std::shared_ptr<ManagedTexture> texture = nullptr, const std::vector<Wall::Side>& skipWalls = {}) {
    Entity room = reg.create();
    reg.add<TransformComp>(room, TransformComp{ pos, size });
    reg.add<WorldTransform>(room, WorldTransform{});
    reg.add<Children>(room, Children{});
    
    Vector3 half = { size.x/2, size.y/2, size.z/2 };
    
    // [] lambda function that captures variables by reference allowing this to change variables/values outside of the scope of the lambda function
    auto makeWall = [&](Vector3 localPos, Vector3 sz, Wall::Side side) -> Entity {
        if (std::find(skipWalls.begin(), skipWalls.end(), side) != skipWalls.end())
            return INVALID_ENTITY;
            
        Entity wall = reg.create();
        reg.add<TransformComp>(wall, TransformComp{ localPos, sz });
        reg.add<WorldTransform>(wall, WorldTransform{});
        
        if (texture) 
            reg.add<TexturedRender>(wall, TexturedRender{ texture });
        else 
            reg.add<ColoredRender>(wall, ColoredRender{ GRAY });
            
        reg.add<Collision>(wall, Collision{});
        reg.add<Parent>(wall, Parent{ room });
        reg.add<Wall>(wall, Wall{ side });
        
        if (auto children = reg.get<Children>(room))
            children->entities.push_back(wall);
            
        return wall;
    };
    
    // floor and ceiling
    makeWall({0, -half.y, 0}, { size.x, 0.1f, size.z }, Wall::Side::Front);
    makeWall({0,  half.y, 0}, { size.x, 0.1f, size.z }, Wall::Side::Back);
    
    // side walls
    makeWall({-half.x, 0, 0}, { 0.1f, size.y, size.z }, Wall::Side::Left);
    makeWall({ half.x, 0, 0}, { 0.1f, size.y, size.z }, Wall::Side::Right);
    
    // front/back walls
    makeWall({0, 0, -half.z}, { size.x, size.y, 0.1f }, Wall::Side::Front);
    makeWall({0, 0,  half.z}, { size.x, size.y, 0.1f }, Wall::Side::Back);
    
    // anchors for connections (all walls have anchors)
    auto addAnchor = [&](Vector3 localPos, Vector3 dir) -> Entity {
        Entity a = reg.create();
        reg.add<TransformComp>(a, TransformComp{ localPos, {0.1f, 0.1f, 0.1f} });
        reg.add<WorldTransform>(a, WorldTransform{});
        reg.add<Anchor>(a, Anchor{ localPos, dir, INVALID_ENTITY });
        reg.add<Parent>(a, Parent{ room });
        
        if (auto children = reg.get<Children>(room)) children->entities.push_back(a);
        
        std::cout << "DEV: Anchor at (" << localPos.x << "," << localPos.y << "," << localPos.z << ") dir (" << dir.x << "," << dir.y << "," << dir.z << ")\n";                  
        return a;
    };
    
    addAnchor({0, 0, -half.z}, {0, 0, -1}); // front
    addAnchor({0, 0, half.z}, {0, 0, 1}); // back
    addAnchor({-half.x, 0, 0}, {-1, 0, 0}); // left
    addAnchor({ half.x, 0, 0}, {1, 0, 0});  // right    
    return room;
}