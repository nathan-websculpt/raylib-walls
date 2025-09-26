#pragma once
#include "../ecs/components.h"
#include "../ecs/registry.h"
#include "../textures/managed_texture.h"
#include <algorithm>
#include <memory>

inline void MakeWallWithDoor(Registry& reg, Entity parent, Vector3 localPos, Vector3 size, std::shared_ptr<ManagedTexture> texture, bool hasDoor = false, float doorWidth = 2.0f, float doorHeight = 3.0f) {
    if (!hasDoor) {
        Entity wall = reg.create();
        reg.add<TransformComp>(wall, TransformComp{ localPos, size });
        
        if (texture) 
            reg.add<TexturedRender>(wall, TexturedRender{ texture });
        else 
            reg.add<ColoredRender>(wall, ColoredRender{ GRAY });
            
        reg.add<Collision>(wall, Collision{});
        reg.add<Parent>(wall, Parent{ parent });
        
        if (auto children = reg.get<Children>(parent)) 
            children->entities.push_back(wall);
        
        return;
    }
    
    float halfW = size.x / 2;
    float halfH = size.y / 2;
    float doorHalfW = doorWidth / 2;
    float doorHalfH = doorHeight / 2;
    
    // left segment
    Entity left = reg.create();
    reg.add<TransformComp>(left, TransformComp{ 
        { localPos.x - (halfW - doorHalfW)/2, localPos.y, localPos.z },
        { halfW - doorHalfW, size.y, size.z } 
    });
    
    if (texture) 
        reg.add<TexturedRender>(left, TexturedRender{ texture });
    else 
        reg.add<ColoredRender>(left, ColoredRender{ GRAY });
        
    reg.add<Collision>(left, Collision{});
    reg.add<Parent>(left, Parent{ parent });
    
    if (auto children = reg.get<Children>(parent)) 
        children->entities.push_back(left);    
    
    // right segment
    Entity right = reg.create();
    reg.add<TransformComp>(right, TransformComp{ 
        { localPos.x + (halfW - doorHalfW)/2, localPos.y, localPos.z },
        { halfW - doorHalfW, size.y, size.z } 
    });
    
    if (texture) 
        reg.add<TexturedRender>(right, TexturedRender{ texture });
    else 
        reg.add<ColoredRender>(right, ColoredRender{ GRAY });
        
    reg.add<Collision>(right, Collision{});
    reg.add<Parent>(right, Parent{ parent });
    
    if (auto children = reg.get<Children>(parent)) 
        children->entities.push_back(right);    
    
    // top segment above door
    Entity top = reg.create();
    reg.add<TransformComp>(top, TransformComp{ 
        { localPos.x, localPos.y + (halfH - doorHalfH)/2, localPos.z },
        { doorWidth, halfH - doorHalfH, size.z } 
    });
    
    if (texture) 
        reg.add<TexturedRender>(top, TexturedRender{ texture });
    else 
        reg.add<ColoredRender>(top, ColoredRender{ GRAY });
        
    reg.add<Collision>(top, Collision{});
    reg.add<Parent>(top, Parent{ parent });
    
    if (auto children = reg.get<Children>(parent)) 
        children->entities.push_back(top);    
        
}