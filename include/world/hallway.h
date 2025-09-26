#pragma once
#include "../ecs/components.h"
#include "../ecs/registry.h"
#include "../textures/managed_texture.h"
#include "room.h"
#include <memory>

// a hallway is just a skinny room
// note: ConnectAnchors will carve openings automatically
inline Entity CreateHallway(Registry& reg, Vector3 pos, Vector3 size,
                         std::shared_ptr<ManagedTexture> texture = nullptr)
{
    Entity hall = reg.create();
    reg.add<TransformComp>(hall, TransformComp{ pos, size });
    reg.add<WorldTransform>(hall, WorldTransform{});
    reg.add<Children>(hall, Children{});
    
    Vector3 half = { size.x/2, size.y/2, size.z/2 }; // TODO: refactor with room?
    
    // create side walls... no front/back walls for a hallway
    auto makeWall = [&](Vector3 localPos, Vector3 sz, Wall::Side side) -> Entity {
        Entity wall = reg.create();
        reg.add<TransformComp>(wall, TransformComp{ localPos, sz });
        reg.add<WorldTransform>(wall, WorldTransform{});
        
        if (texture) 
            reg.add<TexturedRender>(wall, TexturedRender{ texture });
        else 
            reg.add<ColoredRender>(wall, ColoredRender{ GRAY });
            
        reg.add<Collision>(wall, Collision{});
        reg.add<Parent>(wall, Parent{ hall });
        reg.add<Wall>(wall, Wall{ side });
        
        if (auto children = reg.get<Children>(hall))
            children->entities.push_back(wall);
                
        return wall;
    };
    
    // floor and ceiling
    makeWall({0, -half.y, 0}, { size.x, 0.1f, size.z }, Wall::Side::Front);
    makeWall({0,  half.y, 0}, { size.x, 0.1f, size.z }, Wall::Side::Back);
    
    // side walls
    makeWall({-half.x, 0, 0}, { 0.1f, size.y, size.z }, Wall::Side::Left);
    makeWall({ half.x, 0, 0}, { 0.1f, size.y, size.z }, Wall::Side::Right);
    
    auto addAnchor = [&](Vector3 localPos, Vector3 dir) -> Entity {
        Entity a = reg.create();
        reg.add<TransformComp>(a, TransformComp{ localPos, {0.1f, 0.1f, 0.1f} });
        reg.add<WorldTransform>(a, WorldTransform{});
        reg.add<Anchor>(a, Anchor{ localPos, dir, INVALID_ENTITY });
        reg.add<Parent>(a, Parent{ hall });
        
        if (auto children = reg.get<Children>(hall)) 
            children->entities.push_back(a);
             
        return a;
    };
    
    addAnchor({0, 0, -half.z}, {0, 0, -1}); // front
    addAnchor({0, 0, half.z}, {0, 0, 1}); // back
    addAnchor({-half.x, 0, 0}, {-1, 0, 0}); // left
    addAnchor({ half.x, 0, 0}, {1, 0, 0});  // right    
    return hall;
}