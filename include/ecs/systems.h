#pragma once
#include "components.h"
#include "registry.h"
#include "../render/draw_utils.h"
#include "raylib.h"
#include <memory>
#include <unordered_set>
#include <queue>

// this is here, because this system will likely bloat as I code more
class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(Registry& reg, float deltaTime = 0.0f) = 0;
};

// calculates hierarchical world transforms
// based on local transforms (TransformComp) and parent-child relationships
class TransformSystem : public ISystem {
public:
    void update(Registry& reg, float deltaTime = 0.0f) override {
        // build dependency graph to process parents before children
        std::unordered_map<Entity, std::vector<Entity>> parentToChildren;
        std::unordered_set<Entity> rootEntities; // entities without parents

        // notes on std::unordered_set:
        //               ensures each element is unique
        //               elements are not stored in a particular order
        //               uses a hash function to map elements to a specific bucket in the underlying data structure
        //               efficient for large data sets
        
        // collect all entities and their relationships
        for (const auto& entityComp : reg.view<TransformComp>()) {
            Entity e = entityComp.first;
            
            if (auto parent = reg.get<Parent>(e)) {
                parentToChildren[parent->parent].push_back(e);
            } else {
                rootEntities.insert(e);
            }
        }
        
        // process root entities first (no parents)
        for (Entity root : rootEntities) {
            updateEntityTransform(reg, root, INVALID_ENTITY);
        }
        
        // then process children in breadth-first order
        std::queue<Entity> queue;
        for (Entity root : rootEntities) {
            queue.push(root);
        }
        
        // for each entity, retrieve its children from the parentToChildren map 
        // and update their transforms by calling the updateEntityTransform function
        // ... then add the children to the queue for further processing
        while (!queue.empty()) {
            Entity parentEntity = queue.front();
            queue.pop();
            
            // process all children of this parent
            auto it = parentToChildren.find(parentEntity);
            if (it != parentToChildren.end()) {
                for (Entity child : it->second) {
                    updateEntityTransform(reg, child, parentEntity);
                    queue.push(child); // add to queue for processing its children
                }
            }
        }
    }

private:
    void updateEntityTransform(Registry& reg, Entity e, Entity parentEntity) {
        if (auto transform = reg.get<TransformComp>(e)) {
            WorldTransform world{};
            
            if (parentEntity != INVALID_ENTITY && reg.has<WorldTransform>(parentEntity)) {
                // get parent's world transform
                auto parentWorldTransform = reg.get<WorldTransform>(parentEntity);
                if (parentWorldTransform) {
                    // apply proper hierarchical transformation
                    // 1. scale the local position by parent's scale (if needed)
                    Vector3 scaledLocalPos = transform->position;
                    
                    // 2. rotate the local position by parent's rotation
                    Vector3 rotatedLocalPos = scaledLocalPos;
                    
                    // apply rotations in order: Z, X, Y
                    if (parentWorldTransform->rotation.z != 0)
                        rotatedLocalPos = Vector3RotateByAxisAngle(rotatedLocalPos, {0, 0, 1}, parentWorldTransform->rotation.z * DEG2RAD);
                    if (parentWorldTransform->rotation.x != 0)
                        rotatedLocalPos = Vector3RotateByAxisAngle(rotatedLocalPos, {1, 0, 0}, parentWorldTransform->rotation.x * DEG2RAD);
                    if (parentWorldTransform->rotation.y != 0) 
                        rotatedLocalPos = Vector3RotateByAxisAngle(rotatedLocalPos, {0, 1, 0}, parentWorldTransform->rotation.y * DEG2RAD);
                    
                    // 3. translate by parent's world position
                    world.position = Vector3Add(parentWorldTransform->position, rotatedLocalPos);
                    
                    // 4. combine rotations (simple addition for now, but can be improved)
                    // TODO: technically incorrect; use quaternions or matrix multiplication instead of raw Euler addition
                    world.rotation = Vector3Add(parentWorldTransform->rotation, transform->rotation);
                    
                    // 5. for size, could later multiply by parent's scale to add scale support
                    world.size = transform->size;
                } else {
                    // FALLBACK: use local transform if parent world transform doesn't exist
                    world.position = transform->position;
                    world.size = transform->size;
                    world.rotation = transform->rotation;
                }
            } else {
                // root entity - world transform equals local transform
                world.position = transform->position;
                world.size = transform->size;
                world.rotation = transform->rotation;
            }
            
            // update or add WorldTransform component
            if (reg.has<WorldTransform>(e)) 
                *reg.get<WorldTransform>(e) = world;
            else 
                reg.add<WorldTransform>(e, world);

        }
    }
};


// renders all entities with WorldTransform
// iterates over entities with:
//      WorldTransform and either 
//      ColoredRender or TexturedRender components 
class DrawSystem : public ISystem {
public:
    void update(Registry& reg, float deltaTime = 0.0f) override {       
        for (const auto& entityComp : reg.view<WorldTransform>()) {
            Entity e = entityComp.first;
            const WorldTransform& wt = *entityComp.second;
            
            if (auto cr = reg.get<ColoredRender>(e)) 
                DrawCube(wt.position, wt.size.x, wt.size.y, wt.size.z, cr->color); // colored walls
            else if (auto tr = reg.get<TexturedRender>(e))
                if (tr->texture)
                    DrawCubeTexture(tr->texture->get(), wt.position, wt.size.x, wt.size.y, wt.size.z, WHITE); // textured walls
        }
    }
};

// system manager for organizing systems
class SystemManager {
private:
    std::vector<std::unique_ptr<ISystem>> systems;
    Registry& registry;
    
public:
    SystemManager(Registry& reg) : registry(reg) {}
    
    template<typename T, typename... Args>
    T& addSystem(Args&&... args) {
        auto system = std::make_unique<T>(std::forward<Args>(args)...);
        auto& ref = *system;
        systems.push_back(std::move(system));
        return ref;
    }
    
    void update(float deltaTime) {
        for (auto& system : systems)
            system->update(registry, deltaTime);
    }
};