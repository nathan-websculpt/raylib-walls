#pragma once
#include "registry.h"
#include "components.h"

inline void DestroyEntityWithChildren(Registry& reg, Entity e) {
    if (!reg.has<TransformComp>(e)) {
        return;
    }
    
    // remove from parent's children list if this entity has a parent
    if (auto parentComp = reg.get<Parent>(e)) {
        if (auto children = reg.get<Children>(parentComp->parent)) {
            children->entities.erase(
                std::remove(children->entities.begin(), children->entities.end(), e),
                children->entities.end()
            );
        }
    }
    
    // remove all children of this entity
    if (auto children = reg.get<Children>(e)) {
        // create a copy to avoid modifying while iterating
        auto childEntities = children->entities;
        for (Entity child : childEntities) {
            DestroyEntityWithChildren(reg, child);
        }
    }
    
    // destroy the entity
    reg.destroy(e);
}