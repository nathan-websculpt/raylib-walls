#pragma once
#include "raylib.h"

// RAII wrapper for Raylib textures
// No accidental copying
// Walls store only a reference to the texture
class ManagedTexture {
private:
    Texture2D m_texture{};

public:
    ManagedTexture() = default;
    explicit ManagedTexture(const char* filePath) {
        m_texture = LoadTexture(filePath);
    }

    // No copying (textures are GPU resources)
    ManagedTexture(const ManagedTexture&) = delete;
    ManagedTexture& operator=(const ManagedTexture&) = delete;

    // Move semantics
    ManagedTexture(ManagedTexture&& other) noexcept : m_texture(other.m_texture) {
        other.m_texture.id = 0;
    }

    ManagedTexture& operator=(ManagedTexture&& other) noexcept {
        if (this != &other) {
            Unload();
            m_texture = other.m_texture;
            other.m_texture.id = 0;
        }
        return *this;
    }

    ~ManagedTexture() {
        Unload();
    }

    // Access underlying Raylib texture
    [[nodiscard]] Texture2D get() const { return m_texture; }

private:
    void Unload() {
        if (m_texture.id != 0) {
            UnloadTexture(m_texture);
        }
    }
};
