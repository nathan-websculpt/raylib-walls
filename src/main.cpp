#include <memory>
#include "raylib.h"
#include "../include/core/custom_camera.h"
#include "../include/core/wall_handler.h"
#include "../include/core/textured_wall.h"
#include "../include/core/textured_wall_rec.h"
#include "../include/core/colored_wall.h"
#include "../include/core/managed_texture.h"

int main(void)
{
    const int screenWidth = 4400;
    const int screenHeight = 2800;

    InitWindow(screenWidth, screenHeight, "FPS SYSTEM");

    Camera camera { 0 };
    camera.position = (Vector3){ 0.0f, 2.0f, 4.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;                         
    camera.projection = CAMERA_PERSPECTIVE;      

    int cameraMode = CAMERA_FIRST_PERSON;

    DisableCursor();

    SetTargetFPS(60);

    // RAII Texture
    ManagedTexture brick("assets/models/brick/textures/Brick_Wall_5M_Berlin_yhtvxwB_4K_baseColor.png"); 

    WallHandler wallHandler;

    // full-textured wall
    wallHandler.AddWall(std::make_unique<TexturedWall>(
        Vector3{0.0f, 2.5f, -8.0f},
        Vector3{8.0f, 5.0f, 1.0f},
        brick
    ));
    
    // sub-rectangle textured wall (atlas example)
    Rectangle brickFace = {
        0.0f,
        0.0f,
        static_cast<float>(brick.get().width) / 2.0f,
        static_cast<float>(brick.get().height) / 2.0f
    };
    wallHandler.AddWall(std::make_unique<TexturedWallRec>(
        Vector3{10.0f, 2.5f, -8.0f},
        Vector3{4.0f, 5.0f, 1.0f},
        brick,
        brickFace
    ));

    // colored walls
    wallHandler.AddWall(std::make_unique<ColoredWall>(Vector3{-16.0f, 2.5f, 0.0f},Vector3{1.0f, 5.0f, 32.0f}, BLUE));
    wallHandler.AddWall(std::make_unique<ColoredWall>(Vector3{16.0f, 2.5f, 0.0f}, Vector3{1.0f, 5.0f, 32.0f}, LIME));
    wallHandler.AddWall(std::make_unique<ColoredWall>(Vector3{0.0f, 2.5f, 16.0f}, Vector3{32.0f, 5.0f, 1.0f}, GOLD));

    while (!WindowShouldClose())
    {   
        UpdateCamera(&camera, cameraMode); 
        BeginDrawing();

            ClearBackground(RAYWHITE);

            BeginMode3D(camera);

                DrawPlane((Vector3){ 0.0f, 0.0f, 0.0f }, (Vector2){ 32.0f, 32.0f }, LIGHTGRAY); 
                wallHandler.DrawWalls(true);

            EndMode3D();
        EndDrawing();
    }
    
    CloseWindow();

    return 0;
}