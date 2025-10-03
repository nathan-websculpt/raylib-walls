#pragma once
#include "components.h"
#include "registry.h"
#include "../render/draw_utils.h"
#include "raylib.h"
#include <memory>
#include <unordered_set>
#include <queue>

class ISystem {
public:
    virtual ~ISystem() = default;
    virtual void update(Registry& reg, float deltaTime = 0.0f) = 0;
};

// convert euler angles (in degrees) to rotation matrix
// uses YXZ (yaw-pitch-roll) order
Matrix MatrixFromEulerDegrees(Vector3 eulerAngles) {
    float radX = eulerAngles.x * DEG2RAD;
    float radY = eulerAngles.y * DEG2RAD;
    float radZ = eulerAngles.z * DEG2RAD;
    Matrix rotX = MatrixRotate(Vector3{1, 0, 0}, radX);
    Matrix rotY = MatrixRotate(Vector3{0, 1, 0}, radY);
    Matrix rotZ = MatrixRotate(Vector3{0, 0, 1}, radZ);
    // apply in order: Z (roll), then X (pitch), then Y (yaw) so final matrix = Y * X * Z
    Matrix result = MatrixMultiply(MatrixMultiply(rotY, rotX), rotZ);
    return result;
}

// extract Euler angles (in degrees) from rotation matrix
// assumes YXZ (yaw-pitch-roll) order
Vector3 EulerFromMatrix(Matrix mat) {
    // clamp to avoid NaN due to floating point inaccuracies
    const float epsilon = 1e-6f;
    float sy = sqrtf(mat.m0 * mat.m0 + mat.m1 * mat.m1);
    Vector3 eulerAngles;
    if (sy > epsilon) {
        eulerAngles.x = asinf(-mat.m2) * RAD2DEG;
        eulerAngles.y = atan2f(mat.m6, mat.m10) * RAD2DEG;
        eulerAngles.z = atan2f(mat.m1, mat.m0) * RAD2DEG;
    } else {
        // gimbal lock - pitch is +/- 90 degrees
        eulerAngles.x = asinf(-mat.m2) * RAD2DEG;
        eulerAngles.y = 0.0f;
        eulerAngles.z = atan2f(-mat.m9, mat.m5) * RAD2DEG;
    }
    return eulerAngles;
}

// calculates hierarchical world transforms
// based on local transforms (TransformComp) and parent-child relationships
class TransformSystem : public ISystem {
public:
    void update(Registry& reg, float deltaTime = 0.0f) override {
        // build dependency graph to process parents before children
        std::unordered_map<Entity, std::vector<Entity>> parentToChildren;
        std::unordered_set<Entity> rootEntities; // entities without parents
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
                    // build parent's world transformation matrix (rotation + translation)
                    Matrix parentRot = MatrixFromEulerDegrees(parentWorldTransform->rotation);
                    Matrix parentMat = MatrixTranslate(parentWorldTransform->position.x, parentWorldTransform->position.y, parentWorldTransform->position.z);
                    parentMat = MatrixMultiply(parentRot, parentMat); // rotation applied before translation

                    // transform local position by parent's world matrix
                    Vector3 worldPos = Vector3Transform(transform->position, parentMat);

                    // combine rotations via matrix multiplication
                    Matrix localRot = MatrixFromEulerDegrees(transform->rotation);
                    Matrix worldRotMat = MatrixMultiply(parentRot, localRot);
                    Vector3 worldRot = EulerFromMatrix(worldRotMat);

                    // set world transform
                    world.position = worldPos;
                    world.rotation = worldRot;
                    world.size = transform->size;                     
                    // world.size = Vector3Multiply(parentWorldTransform->size, transform->size); // TODO: if hierarchical scaling is needed, multiply by parent size
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