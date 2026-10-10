#pragma once
// Выпадающие бонусы: аптечки, патроны, эссенция.
#include "Common.h"
#include "Player.h"
#include "World.h"

enum class PickupType { Health, Ammo, Essence };

struct Pickup {
    PickupType type = PickupType::Essence;
    Vector2 pos{ 0.0f, 0.0f };
    Vector2 vel{ 0.0f, 0.0f };
    int amount = 1;
    float life = 30.0f;
    float age = 0.0f;
    bool alive = true;

    static Pickup Make(PickupType type, Vector2 pos, int amount);
    // Возвращает true, если игрок подобрал бонус в этом кадре.
    bool Update(float dt, const World& world, const Player& player);
    bool Wants(const Player& player) const;
    void Draw() const;
};
