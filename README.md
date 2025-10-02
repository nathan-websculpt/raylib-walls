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
	


	



