#pragma once
#include "raylib.h"

// RAII wrapper for raylib textures (no accidental copying)
class ManagedTexture {
private:
    Texture2D texture{};

public:
    ManagedTexture() = default;
    explicit ManagedTexture(const char* filePath) {
        texture = LoadTexture(filePath);
    }

    // no copying (textures are GPU resources)
    ManagedTexture(const ManagedTexture&) = delete;
    ManagedTexture& operator=(const ManagedTexture&) = delete;

    // move semantics
    ManagedTexture(ManagedTexture&& other) noexcept : texture(other.texture) {
        other.texture.id = 0;
    }

    ManagedTexture& operator=(ManagedTexture&& other) noexcept {
        if (this != &other) {
            Unload();
            texture = other.texture;
            other.texture.id = 0;
        }
        return *this;
    }

    ~ManagedTexture() {
        Unload();
    }

    // access underlying raylib texture
    [[nodiscard]] Texture2D get() const { return texture; }

private:
    void Unload() {
        if (texture.id != 0) {
            UnloadTexture(texture);
        }
    }
};
