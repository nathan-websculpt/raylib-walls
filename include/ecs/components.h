#pragma once
#include "raylib.h"
#include <vector>
#include <memory>
#include "../textures/managed_texture.h"

struct TransformComp {
    Vector3 position{0};
    Vector3 size{1, 1, 1};
    Vector3 rotation{0}; // in degrees
    
    TransformComp() = default;
    TransformComp(Vector3 pos, Vector3 sz) : position(pos), size(sz), rotation{0, 0, 0} {}
    TransformComp(Vector3 pos, Vector3 sz, Vector3 rot) : position(pos), size(sz), rotation(rot) {}
};

// TODO: should WorldTransforms that are only needed when the game loads be calculated on-the-fly?
struct WorldTransform {
    Vector3 position{0};
    Vector3 size{1, 1, 1};
    Vector3 rotation{0}; // in degrees
    
    WorldTransform() = default;
    WorldTransform(Vector3 pos, Vector3 sz) : position(pos), size(sz), rotation{0, 0, 0} {}
    WorldTransform(Vector3 pos, Vector3 sz, Vector3 rot) : position(pos), size(sz), rotation(rot) {}
};

// note: the parent member variable stores the ID of the parent entity
struct Parent {
    Entity parent{INVALID_ENTITY};
    
    Parent() = default;
    Parent(Entity p) : parent(p) {}
};

struct Children {
    std::vector<Entity> entities;
    
    Children() = default;
    Children(std::vector<Entity> e) : entities(std::move(e)) {}
    
    const std::vector<Entity>& getEntities() const { return entities; }
};

struct ColoredRender {
    Color color{WHITE};
    
    ColoredRender() = default;
    ColoredRender(Color c) : color(c) {}
};

struct TexturedRender {
    std::shared_ptr<ManagedTexture> texture;
    
    TexturedRender() = default;
    TexturedRender(std::shared_ptr<ManagedTexture> tex) : texture(std::move(tex)) {}
    
    const std::shared_ptr<ManagedTexture>& getTexture() const { return texture; }
};

struct Collision {
    bool enabled{true};
    
    Collision() = default;
    Collision(bool enable) : enabled(enable) {}
    
    bool isEnabled() const { return enabled; }
};

struct Wall {
    enum class Side { Front, Back, Left, Right };
    Side side{Side::Front};
    
    Wall() = default;
    Wall(Side s) : side(s) {}
    
    Side getSide() const { return side; }
};

struct Anchor {
    Vector3 localPos{0};
    Vector3 direction{0, 0, 1}; // normalized
    Entity connectedTo{INVALID_ENTITY};   // the other anchor entity id
    
    Anchor() = default;
    Anchor(Vector3 pos, Vector3 dir, Entity connected) 
        : localPos(pos), direction(dir), connectedTo(connected) {}
    
    const Vector3& getLocalPos() const { return localPos; }
    const Vector3& getDirection() const { return direction; }
    Entity getConnectedTo() const { return connectedTo; }
};