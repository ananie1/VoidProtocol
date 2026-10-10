#pragma once
// Снаряд (общий для игрока и врагов).
#include "Common.h"

struct Projectile {
    Vector2 pos{ 0.0f, 0.0f };
    Vector2 vel{ 0.0f, 0.0f };
    float radius = 4.0f;
    float damage = 1.0f;
    float life = 1.0f;       // оставшееся время жизни, сек
    bool fromPlayer = false;
    bool alive = true;
    Color color = WHITE;

    void Draw() const {
        if (fromPlayer) {
            DrawLineEx(pos, pos - vel * 0.022f, radius * 1.8f, Fade(color, 0.45f));
            DrawCircleV(pos, radius, color);
            DrawCircleV(pos, radius * 0.5f, WHITE);
        } else {
            DrawCircleV(pos, radius + 4.0f, Fade(color, 0.25f));
            DrawCircleV(pos, radius, color);
            DrawCircleV(pos, radius * 0.45f, WHITE);
        }
    }
};
