#### NOTE: This has only been tested on Linux (on wide monitors) - requires raylib

# dev questions/todo

- Avoid std::type_index — Use Type IDs via Templates instead
    - std::type_index has runtime overhead and isn’t constexpr
- current intersectEntities() builds a std::unordered_set per query — O(N) memory + hash overhead
    - Merge sorted dense entity lists? Since dense_entities is not sorted by ID, you can’t do a classic merge... but you could sort it once per frame...
- Should WorldTransform be computed on-the-fly instead of existing as components
    - some objects will only be transformed on init...
- version assignment wrap-around could eventually break, if the world got huge
- still having problems with multi-component views
    - should use sorted dense arrays + merge (but right now, generic views aren't needed)
- Replace RTTI in getPool() with compile-time component IDs?
- note: not done yet, so rendering currently ignores rotation

# Compile
```
g++ -std=c++23 main.cpp src/render/draw_utils.cpp -o main -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

./main
```


# running tests 
#### (ensure GLFW, Raylib, and GTest are installed)

```bash
# Make the script executable
chmod +x build_tests.sh

# Run the tests
./build_tests.sh

```


## BRANCH NOTES

Has been changed from a class-based (polymorphic) system to an ECS system. 

# The ECS

* Uses a sparse array (indexed by entity ID) that points to indices in dense arrays (entities + components).  
* Deletion swaps the target with the last element in the dense arrays ( pops the last ), and updates the sparse entry for the moved entity, and marks the deleted entity as absent in the sparse array.  
    * When an entity is destroyed: 
        * Its ID is put back into a freeIds queue for reuse.
        But its version is incremented.
            * If version hits max (255), it wraps back to 1.
         

    * When you try to use an old Entity handle: 
        * The system checks: "Does the current version for this ID match the handle’s version?"
            * If no → the handle is stale (refers to a deleted entity) and access is denied.

#### What Entity Versions solves

An `Entity` is a **Versioned Handle** to prevent *Use-After-Free* problems

Without Versions:
```cpp
Entity e = registry.create();   // e.id = 5
registry.destroy(e);
Entity f = registry.create();   // reuses id = 5
// Now e and f have same ID - using e accidentally accesses f!!!
```

With Versions:
```cpp
Entity e = {id:5, ver:1}
registry.destroy(e);            // version for id = 5 becomes 2
Entity f = {id:5, ver:2}        // new entity
// Now e != f
```
         
     
	

# Rooms && Walls

`TransformComp`: Local position, size, and rotation

`WorldTransform`: World position, size, and rotation (computed)

`ColoredRender`/`TexturedRender`: Rendering components

`Wall`: Identifies wall entities and their orientation

`Anchor`: Connection points between rooms and hallways
	
A `room` is an entity with:

* `TransformComp` and `WorldTransform` for position and size.
* `Children` containing its walls and anchors.

`Walls` are child entities, each with:

* `TransformComp` defining their local position and thickness.
* `Wall::Side` metadata (Front, Back, Left, Right)... currently used to omit walls
* Either a texture (`TexturedRender`) or a flat color (`ColoredRender`).

Skipped walls: `CreateRoom` takes a list of `skipWalls`, omitting entire sides if needed (open front/back/etc).

#### Anchors

Each room automatically generates `anchors` in the center of its four directions. These serve as connection points for Hallways.

A `hallway` is essentially a thin room.

`Anchors` are tiny child entities with:

* `TransformComp` (local offset at wall midpoint).
* `Anchor` (stores direction vector + optional connectedTo reference).

# In Game

- Aim fast with arrow keys
- Aim slow with I, J, K, L