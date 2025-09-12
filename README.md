#### NOTE: This has only been tested on Linux (on wide monitors) - requires raylib

# Compile
```
g++ -std=c++23 src/main.cpp src/core/wall.cpp src/core/wall_handler.cpp src/core/textured_wall.cpp src/core/textured_wall_rec.cpp src/core/colored_wall.cpp src/core/draw_utils.cpp -o main -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

./main
```

### Notes

- Each `Wall` is a class instance with its own `BoundingBox`
- `WallHandler` manages all `Walls`
- Analyze `BoundingBoxes` later via `WallHandler.walls`

## Hierarchy

```
│
├── main.cpp
│   └─ uses: WallHandler, ManagedTexture, Raylib
│
├── wall/                     <-- Wall base class and derivatives
│   ├── wall.h                <-- abstract Wall class
│   ├── wall.cpp              <-- (optional, may contain Wall implementation)
│   ├── colored_wall.h        <-- ColoredWall
│   ├── colored_wall.cpp
│   ├── textured_wall.h       <-- TexturedWall
│   ├── textured_wall.cpp
│   ├── textured_wall_rec.h   <-- TexturedWallRec (texture with a portion of the image)
│   └── textured_wall_rec.cpp
│
├── wall_handler/             <-- Manages a collection of Walls
│   ├── wall_handler.h
│   └── wall_handler.cpp
│
├── managed_texture/           <-- RAII Texture wrapper
│   ├── managed_texture.h
│   └── managed_texture.cpp
│
└── draw_utils/               <-- Utility drawing functions for cubes
    ├── draw_utils.h
    └── draw_utils.cpp


```

## Dependencies / Relationships

- main.cpp
    - Creates walls using ColoredWall, TexturedWall, TexturedWallRec.
    - Uses WallHandler to store and draw walls.
    - Uses ManagedTexture to load textures safely.
- Wall hierarchy (wall/)
    - Wall is the base class.
    - Derived classes override Draw() and optionally DrawDebug().
- WallHandler
    - Stores std::unique_ptr<Wall> in a container.
    - Calls polymorphic Draw() on each wall.
- ManagedTexture
    - Provides RAII-style management of Texture2D.
    - Textured walls use Texture2D or ManagedTexture indirectly.
- Draw utils
    - Low-level cube drawing functions.
    - Used by textured walls or optionally by Wall derivatives.

## Diagram

```
main.cpp
  ├─ uses WallHandler
  │     └─ stores std::unique_ptr<Wall> -> Wall hierarchy
  │           ├─ ColoredWall
  │           ├─ TexturedWall
  │           └─ TexturedWallRec
  │
  └─ uses ManagedTexture -> provides safe Texture2D management

Wall hierarchy (wall/)
  └─ Wall (abstract base)
      ├─ ColoredWall
      ├─ TexturedWall
      └─ TexturedWallRec

DrawUtils
  └─ provides DrawCubeTexture / DrawCubeTextureRec for walls

ManagedTexture
  └─ RAII wrapper for Texture2D


```

#### other

- Polymorphic wall hierarchy is fully contained under `wall/`.
- `ColoredWall`, `TexturedWall`, `TexturedWallRec` inherit and override `Draw()`
- Memory safety is partly handled (via `WallHandler` smart pointers and `ManagedTexture`)
- `WallHandler` stores `std::unique_ptr<Wall>` and calls `Draw()` polymorphically
- `WallHandler` owns all walls via `std::unique_ptr`
- Textures are managed via `std::shared_ptr<Texture2D>` (or `ManagedTexture`) to allow sharing between walls safely
- RAII
    - `ManagedTexture` ensures `Texture2D` is automatically unloaded when no longer used.
    - `std::unique_ptr` ensures walls are deleted when `WallHandler` goes out of scope.

##### diagram

```
+-----------------------+
|       main.cpp        |
|-----------------------|
| - creates walls       |
| - adds to WallHandler |
| - owns ManagedTexture  |
+-----------+-----------+
            |
            v
+---------------------------------------------+
|    WallHandler                              |
|---------------------------------------------|
| std::vector<std::unique_ptr<Wall>>          |
|---------------------------------------------|
| - DrawWalls() calls polymorphic Draw()      |
+-----------+---------------------------------+
            |
            v
   Polymorphic Walls
+-----------------------+
|        Wall           |  <-- abstract base class
|-----------------------|
| +Draw() = 0           |
| +DrawDebug()          |
+-----------+-----------+
            |
   ------------------------
   |           |          |
   v           v          v
+-----------+ +-----------+ +-------------------+
| ColoredWall| |TexturedWall| | TexturedWallRec |
|-----------| |-----------| |-------------------|
| Color m_c | |shared_ptr< | | shared_ptr<      |
|           | | Texture2D> | | Texture2D>       |
+-----------+ +-----------+ +-------------------+

Texture Management (RAII)
+-------------------------+
|     ManagedTexture       |
|-------------------------|
| - owns Texture2D        |
| - RAII unloads texture  |
+-------------------------+
          ^
          |
   shared_ptr used by
   TexturedWall / Rec


```