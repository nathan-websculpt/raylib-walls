#### NOTE: This has only been tested on Linux (on wide monitors) - requires raylib

# Compile
```
g++ -std=c++23 main.cpp src/render/draw_utils.cpp -o main -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

./main
```

## NOTES

Has been changed from a class-based (polymorphic) system to an ECS system. 