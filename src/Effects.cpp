#include "Effects.h"

// ------------------------------------------------------------ ParticleSystem
void ParticleSystem::Emit(Vector2 pos, Vector2 vel, float life, float size, Color color, float drag) {
    if (particles_.size() >= kMaxParticles) return;
    Particle p;
    p.pos = pos;
    p.vel = vel;
    p.life = life;
    p.maxLife = life;
    p.size = size;
    p.drag = drag;
    p.color = color;
    particles_.push_back(p);
}

void ParticleSystem::Burst(Vector2 pos, int count, float speedMin, float speedMax, float sizeMin,
                           float sizeMax, float lifeMin, float lifeMax, Color color, float drag) {
    for (int i = 0; i < count; ++i) {
        const Vector2 dir = rng::Direction();
        Emit(pos, dir * rng::Range(speedMin, speedMax), rng::Range(lifeMin, lifeMax),
             rng::Range(sizeMin, sizeMax), color, drag);
    }
}

void ParticleSystem::Cone(Vector2 pos, Vector2 dir, float spread, int count, float speedMin,
                          float speedMax, float sizeMin, float sizeMax, float lifeMin, float lifeMax,
                          Color color, float drag) {
    const float base = mu::AngleOf(dir);
    for (int i = 0; i < count; ++i) {
        const Vector2 d = mu::FromAngle(base + rng::Range(-spread, spread));
        Emit(pos, d * rng::Range(speedMin, speedMax), rng::Range(lifeMin, lifeMax),
             rng::Range(sizeMin, sizeMax), color, drag);
    }
}

void ParticleSystem::Update(float dt) {
    for (std::size_t i = 0; i < particles_.size();) {
        Particle& p = particles_[i];
        p.life -= dt;
        if (p.life <= 0.0f) {
            p = particles_.back();
            particles_.pop_back();
            continue;
        }
        p.pos += p.vel * dt;
        p.vel *= std::exp(-p.drag * dt);
        ++i;
    }
}

void ParticleSystem::Draw() const {
    BeginBlendMode(BLEND_ADDITIVE);
    for (const Particle& p : particles_) {
        const float t = p.life / p.maxLife;
        const float s = p.size * (0.25f + 0.75f * t);
        DrawCircleV(p.pos, s, Fade(p.color, t));
    }
    EndBlendMode();
}

// --------------------------------------------------------------- ScreenShake
void ScreenShake::Update(float dt) {
    trauma_ = std::max(0.0f, trauma_ - 1.7f * dt);
    const float power = trauma_ * trauma_;
    constexpr float kMaxOffset = 20.0f;
    offset_ = Vector2{ rng::Range(-1.0f, 1.0f) * kMaxOffset * power,
                       rng::Range(-1.0f, 1.0f) * kMaxOffset * power };
}

// ------------------------------------------------------------------- Effects
void Effects::Update(float dt) {
    particles.Update(dt);
    shake.Update(dt);
    for (std::size_t i = 0; i < shocks_.size();) {
        shocks_[i].life -= dt;
        if (shocks_[i].life <= 0.0f) {
            shocks_[i] = shocks_.back();
            shocks_.pop_back();
        } else {
            ++i;
        }
    }
}

void Effects::Draw() const {
    BeginBlendMode(BLEND_ADDITIVE);
    for (const Shockwave& s : shocks_) {
        const float t = 1.0f - s.life / s.maxLife;  // 0 -> 1
        const float r = s.maxRadius * (1.0f - (1.0f - t) * (1.0f - t));
        DrawRing(s.pos, std::max(0.0f, r - 4.0f * (1.0f - t) - 1.0f), r, 0.0f, 360.0f, 40,
                 Fade(s.color, 0.8f * (1.0f - t)));
    }
    EndBlendMode();
    particles.Draw();
}

void Effects::Reset() {
    particles.Clear();
    shake.Reset();
    shocks_.clear();
}

void Effects::Explosion(Vector2 pos, float scale, Color color) {
    particles.Burst(pos, static_cast<int>(28 * scale), 80.0f * scale, 380.0f * scale, 2.0f, 5.5f * scale,
                    0.3f, 0.85f, color, 2.4f);
    particles.Burst(pos, static_cast<int>(14 * scale), 40.0f, 220.0f * scale, 3.0f, 7.0f * scale, 0.2f,
                    0.5f, Color{ 255, 240, 200, 255 }, 3.0f);
    Shock(pos, 55.0f * scale, 0.45f, color);
}

void Effects::Hit(Vector2 pos, Vector2 dir, Color color) {
    particles.Cone(pos, dir, 0.9f, 7, 70.0f, 280.0f, 1.5f, 3.5f, 0.12f, 0.34f, color, 4.0f);
}

void Effects::Muzzle(Vector2 pos, Vector2 dir, Color color) {
    particles.Cone(pos, dir, 0.3f, 4, 120.0f, 340.0f, 1.5f, 3.0f, 0.06f, 0.15f, color, 4.0f);
}

void Effects::Sparkle(Vector2 pos, Color color) {
    particles.Burst(pos, 12, 40.0f, 170.0f, 1.5f, 3.5f, 0.25f, 0.55f, color, 3.5f);
}

void Effects::SpawnPortal(Vector2 pos, Color color) {
    particles.Burst(pos, 16, 30.0f, 130.0f, 1.5f, 3.0f, 0.3f, 0.65f, color, 2.5f);
    Shock(pos, 44.0f, 0.5f, color);
}

void Effects::Shock(Vector2 pos, float maxRadius, float life, Color color) {
    Shockwave s;
    s.pos = pos;
    s.maxRadius = maxRadius;
    s.life = life;
    s.maxLife = life;
    s.color = color;
    shocks_.push_back(s);
}
