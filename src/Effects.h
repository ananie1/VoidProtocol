#pragma once
// Эффекты: система частиц, тряска экрана, ударные волны.
#include "Common.h"

#include <cstddef>
#include <vector>

struct Particle {
    Vector2 pos{ 0.0f, 0.0f };
    Vector2 vel{ 0.0f, 0.0f };
    float life = 0.0f;
    float maxLife = 1.0f;
    float size = 2.0f;
    float drag = 3.0f;
    Color color = WHITE;
};

class ParticleSystem {
public:
    void Emit(Vector2 pos, Vector2 vel, float life, float size, Color color, float drag = 3.0f);
    // Круговой взрыв частиц.
    void Burst(Vector2 pos, int count, float speedMin, float speedMax, float sizeMin, float sizeMax,
               float lifeMin, float lifeMax, Color color, float drag = 3.0f);
    // Направленный выброс (конус).
    void Cone(Vector2 pos, Vector2 dir, float spread, int count, float speedMin, float speedMax,
              float sizeMin, float sizeMax, float lifeMin, float lifeMax, Color color, float drag = 3.0f);
    void Update(float dt);
    void Draw() const;
    void Clear() { particles_.clear(); }
    std::size_t Count() const { return particles_.size(); }

private:
    static constexpr std::size_t kMaxParticles = 3500;
    std::vector<Particle> particles_;
};

class ScreenShake {
public:
    void AddTrauma(float amount) { trauma_ = std::min(1.0f, trauma_ + amount); }
    void Update(float dt);
    void Reset() { trauma_ = 0.0f; offset_ = Vector2{ 0.0f, 0.0f }; }
    Vector2 Offset() const { return offset_; }
    float Trauma() const { return trauma_; }

private:
    float trauma_ = 0.0f;
    Vector2 offset_{ 0.0f, 0.0f };
};

struct Shockwave {
    Vector2 pos{ 0.0f, 0.0f };
    float maxRadius = 60.0f;
    float life = 0.4f;
    float maxLife = 0.4f;
    Color color = WHITE;
};

class Effects {
public:
    ParticleSystem particles;
    ScreenShake shake;

    void Update(float dt);
    void Draw() const;  // вызывать внутри BeginMode2D
    void Reset();

    void Explosion(Vector2 pos, float scale, Color color);
    void Hit(Vector2 pos, Vector2 dir, Color color);
    void Muzzle(Vector2 pos, Vector2 dir, Color color);
    void Sparkle(Vector2 pos, Color color);
    void SpawnPortal(Vector2 pos, Color color);
    void Shock(Vector2 pos, float maxRadius, float life, Color color);

private:
    std::vector<Shockwave> shocks_;
};
