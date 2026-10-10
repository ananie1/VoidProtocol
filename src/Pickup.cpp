#include "Pickup.h"

Pickup Pickup::Make(PickupType type, Vector2 pos, int amount) {
    Pickup p;
    p.type = type;
    p.pos = pos;
    p.amount = amount;
    p.vel = rng::Direction() * rng::Range(60.0f, 210.0f);
    p.life = (type == PickupType::Essence) ? 50.0f : 30.0f;
    p.age = rng::Range(0.0f, 6.0f);
    return p;
}

bool Pickup::Wants(const Player& pl) const {
    switch (type) {
        case PickupType::Health: return pl.NeedsHealth();
        case PickupType::Ammo: return pl.NeedsAmmo();
        case PickupType::Essence: return true;
    }
    return false;
}

bool Pickup::Update(float dt, const World& world, const Player& pl) {
    age += dt;
    life -= dt;
    if (life <= 0.0f) {
        alive = false;
        return false;
    }

    if (pl.alive && Wants(pl)) {
        const float d = mu::Dist(pos, pl.pos);
        const float magnet = (type == PickupType::Essence) ? 150.0f : 70.0f;
        if (d < magnet) vel += mu::Norm(pl.pos - pos) * (1500.0f * dt);  // притяжение
        if (d < pl.radius + 13.0f) return true;
    }

    vel *= std::exp(-4.0f * dt);
    world.MoveCircle(pos, vel * dt, 6.0f);
    return false;
}

void Pickup::Draw() const {
    if (life < 4.0f && std::fmod(life * 8.0f, 2.0f) < 1.0f) return;  // мигание перед исчезновением
    const float bob = std::sin(age * 4.0f) * 2.5f;
    const Vector2 p{ pos.x, pos.y + bob };
    const int x = static_cast<int>(p.x);
    const int y = static_cast<int>(p.y);

    switch (type) {
        case PickupType::Health: {
            const Color c = pal::kHealth;
            DrawCircleV(p, 18.0f, Fade(c, 0.16f));
            DrawRectangleRounded(Rectangle{ p.x - 10.0f, p.y - 10.0f, 20.0f, 20.0f }, 0.3f, 4, Color{ 10, 38, 24, 255 });
            DrawRectangleLinesEx(Rectangle{ p.x - 10.0f, p.y - 10.0f, 20.0f, 20.0f }, 1.5f, c);
            DrawRectangle(x - 2, y - 6, 4, 12, c);
            DrawRectangle(x - 6, y - 2, 12, 4, c);
            break;
        }
        case PickupType::Ammo: {
            const Color c = pal::kAmmo;
            DrawCircleV(p, 18.0f, Fade(c, 0.16f));
            DrawRectangleRounded(Rectangle{ p.x - 10.0f, p.y - 10.0f, 20.0f, 20.0f }, 0.3f, 4, Color{ 46, 38, 8, 255 });
            DrawRectangleLinesEx(Rectangle{ p.x - 10.0f, p.y - 10.0f, 20.0f, 20.0f }, 1.5f, c);
            DrawRectangle(x - 7, y - 6, 4, 12, c);
            DrawRectangle(x - 2, y - 6, 4, 12, c);
            DrawRectangle(x + 3, y - 6, 4, 12, c);
            break;
        }
        case PickupType::Essence: {
            const Color c = pal::kEssence;
            const float pulse = 0.5f + 0.5f * std::sin(age * 6.0f);
            DrawCircleV(p, 12.0f + 3.0f * pulse, Fade(c, 0.18f));
            DrawPoly(p, 4, 7.0f + 2.0f * pulse, age * 120.0f, c);
            DrawPoly(p, 4, 3.5f, age * 120.0f, WHITE);
            break;
        }
    }
}
