#include "raylib.h"
#include <memory>
#include <iostream>
#include "include/core/custom_camera.h"
#include "include/ecs/registry.h"
#include "include/ecs/systems.h"
#include "include/world/room.h"
#include "include/world/hallway.h"
#include "include/world/anchor.h"
#include "include/textures/managed_texture.h"
#include "include/render/draw_utils.h"

// g++ -std=c++23 main.cpp src/render/draw_utils.cpp -o main -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

int main(void)
{
    const int screenWidth = 4400;
    const int screenHeight = 2800;

    InitWindow(screenWidth, screenHeight, "RAYLIB WALLS");

    Camera camera { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;                         
    camera.projection = CAMERA_PERSPECTIVE;      

    int cameraMode = CAMERA_FIRST_PERSON;

    DisableCursor();

    SetTargetFPS(60);

    auto brick = std::make_shared<ManagedTexture>("assets/models/brick/textures/Brick_Wall_5M_Berlin_yhtvxwB_4K_baseColor.png");
    if (brick->get().id == 0) {
        std::cerr << "DEV: Texture failed to load!\n";
        // TODO:
    }
    
    Registry reg;
    
    // TODO: when I figure out what this will become, I will bring the SystemManager back in...
    TransformSystem transformSystem;
    DrawSystem drawSystem;
    
    // scale factor
    const float S = 20.0f;

    // room and hallway dimensions
    Vector3 roomSize = {10*S, 2.5f*S, 10*S};  // 200 x 50 x 200
    Vector3 hallSize = {4*S, 2.5f*S, 12*S};   // 240 x 50 x 80  (long along Z)

    // spacing for front/back alignment
    float spacing = roomSize.z/2 + hallSize.z/2; // 200/2 + 80/2 = 140

    // place rooms in front and back, hallway in between
    Entity room1 = CreateRoom(reg, { 0, 0, -spacing }, roomSize, brick, std::vector<Wall::Side>{ Wall::Side::Back });  // open back
    Entity room2 = CreateRoom(reg, { 0, 0,  spacing }, roomSize, brick, std::vector<Wall::Side>{ Wall::Side::Front }); // open front    
    Entity hall  = CreateHallway(reg, { 0, 0, 0 }, hallSize, brick);

    // now calc WorldTransforms for all entities and anchors
    transformSystem.update(reg);

    // helper to find anchor on a parent with a specific direction
    // used to locate the specific anchor entities on room1, room2, and hall based on their intended connection directions
    auto findAnchorByDir = [&](Entity parentEntity, Vector3 dir)->Entity {
        if (auto children = reg.get<Children>(parentEntity)) {
            for (Entity child : children->entities) {
                // if the child entity has an anchor component, it normalizes both the given direction and the anchor direction
                if (auto a = reg.get<Anchor>(child)) {
                    // normalize and compare directions
                    Vector3 normalizedDir = Vector3Normalize(dir);
                    Vector3 normalizedAnchorDir = Vector3Normalize(a->direction);
                    
                    float dot = Vector3DotProduct(normalizedAnchorDir, normalizedDir);
                    if (dot > 0.99f) return child; // the directions are close enough
                }
            }
        }
        // if no suitable anchor entity is found, return INVALID_ENTITY
        return INVALID_ENTITY;
    };

    // connect room1's right anchor to hall's left anchor
    Entity r1_right = findAnchorByDir(room1, { 1, 0, 0 });
    Entity hall_left = findAnchorByDir(hall, { -1, 0, 0 });


    // ConnectAnchors is called twice 
    // in order to link room1 to hall and room2 to hall 
    // this snaps positions and carves doorways
    if (r1_right != INVALID_ENTITY && hall_left != INVALID_ENTITY) {
        ConnectAnchors(reg, r1_right, hall_left);
    } else {
        std::cerr << "DEV Warning: missing anchors for room1<->hall connection\n";
    }

    // connect room2's left anchor to hall's right anchor
    Entity r2_left = findAnchorByDir(room2, { -1, 0, 0 });
    Entity hall_right = findAnchorByDir(hall, { 1, 0, 0 });

    if (r2_left != INVALID_ENTITY && hall_right != INVALID_ENTITY) {
        ConnectAnchors(reg, r2_left, hall_right);
    } else {
        std::cerr << "DEV Warning: missing anchors for room2<->hall connection\n";
    }

    // called again to update transforms after the connection adjustments
    transformSystem.update(reg);

    while (!WindowShouldClose())
    {   
        UpdateCamera(&camera, cameraMode); 

        float dt = GetFrameTime();

        // periodic cleanup about every 1000 frames
        static int frameCount = 0;
        if (++frameCount % 1000 == 0) {
            reg.cleanup();
        }
        // transformSystem.update(reg, dt); // currently nothing moves, but system supports it

        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                drawSystem.update(reg, dt); // draw all entities (wall system)

            EndMode3D();
        EndDrawing();
    }
    
    CloseWindow();

    return 0;
}