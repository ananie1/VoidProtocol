#include "Player.h"

// ------------------------------------------------------------------ Upgrades
const char* UpgradeName(Upgrade u) {
    switch (u) {
        case Upgrade::Damage: return "OVERCHARGE";
        case Upgrade::FireRate: return "RAPID CYCLE";
        case Upgrade::MaxHealth: return "NANO ARMOR";
        case Upgrade::MoveSpeed: return "SERVO BOOST";
        case Upgrade::MagSize: return "EXTENDED MAG";
        case Upgrade::Reload: return "QUICK LOADER";
    }
    return "";
}

const char* UpgradeDesc(Upgrade u) {
    switch (u) {
        case Upgrade::Damage: return "+18% weapon damage";
        case Upgrade::FireRate: return "+14% fire rate";
        case Upgrade::MaxHealth: return "+25 max HP and heal 25";
        case Upgrade::MoveSpeed: return "+7% move speed";
        case Upgrade::MagSize: return "+8 magazine, +30 reserve";
        case Upgrade::Reload: return "-20% reload time";
    }
    return "";
}

Color UpgradeColor(Upgrade u) {
    switch (u) {
        case Upgrade::Damage: return Color{ 255, 90, 90, 255 };
        case Upgrade::FireRate: return Color{ 255, 190, 70, 255 };
        case Upgrade::MaxHealth: return Color{ 90, 255, 130, 255 };
        case Upgrade::MoveSpeed: return Color{ 90, 210, 255, 255 };
        case Upgrade::MagSize: return Color{ 255, 230, 110, 255 };
        case Upgrade::Reload: return Color{ 190, 130, 255, 255 };
    }
    return WHITE;
}

// -------------------------------------------------------------------- Player
void Player::Reset(Vector2 spawn) {
    *this = Player();
    pos = spawn;
}

void Player::StartReload() {
    if (IsReloading() || mag >= magSize || reserve <= 0) return;
    reloadTimer = reloadTime;
}

void Player::Shoot(std::vector<Projectile>& bullets, Effects& fx) {
    fireCooldown = fireDelay;
    --mag;
    const float ang = mu::AngleOf(aimDir) + rng::Range(-0.03f, 0.03f);
    const Vector2 dir = mu::FromAngle(ang);

    Projectile b;
    b.pos = pos + dir * (radius + 8.0f);
    b.vel = dir * bulletSpeed;
    b.radius = 4.0f;
    b.damage = damage;
    b.life = 0.95f;
    b.fromPlayer = true;
    b.color = pal::kPlayer;
    bullets.push_back(b);

    fx.Muzzle(b.pos, dir, pal::kPlayer);
    fx.shake.AddTrauma(0.035f);
    vel -= dir * 30.0f;  // лёгкая отдача
}

void Player::Update(float dt, const PlayerInput& in, const World& world,
                    std::vector<Projectile>& bullets, Effects& fx) {
    if (!alive) return;
    flashTimer = std::max(0.0f, flashTimer - dt);
    invulnTimer = std::max(0.0f, invulnTimer - dt);
    fireCooldown = std::max(0.0f, fireCooldown - dt);
    dashCooldown = std::max(0.0f, dashCooldown - dt);

    const Vector2 toAim = in.aimWorld - pos;
    if (mu::LenSq(toAim) > 9.0f) aimDir = mu::Norm(toAim);

    // --- Движение и рывок
    if (dashTimer > 0.0f) {
        dashTimer -= dt;
        vel = dashDir * dashSpeed;
        trailTimer_ -= dt;
        if (trailTimer_ <= 0.0f) {
            trailTimer_ = 0.012f;
            fx.particles.Emit(pos, Vector2{ 0.0f, 0.0f }, 0.3f, radius * 0.85f, pal::kPlayer, 0.0f);
        }
    } else {
        if (in.dash && dashCooldown <= 0.0f) {
            Vector2 d = mu::Norm(in.move);
            if (mu::LenSq(d) < 0.01f) d = aimDir;
            dashDir = d;
            dashTimer = dashTime;
            dashCooldown = dashCooldownMax;
            invulnTimer = std::max(invulnTimer, dashTime + 0.08f);
            fx.particles.Burst(pos, 12, 60.0f, 220.0f, 2.0f, 4.0f, 0.2f, 0.4f, pal::kPlayer, 3.0f);
        } else {
            const Vector2 desired = mu::Norm(in.move) * moveSpeed;
            const float k = 1.0f - std::exp(-accel * dt);
            vel += (desired - vel) * k;
        }
    }
    world.MoveCircle(pos, vel * dt, radius);

    // --- Перезарядка
    if (IsReloading()) {
        reloadTimer -= dt;
        if (reloadTimer <= 0.0f) {
            reloadTimer = 0.0f;
            const int take = std::min(magSize - mag, reserve);
            mag += take;
            reserve -= take;
        }
    } else if (in.reload && !IsDashing()) {
        StartReload();
    }

    // --- Стрельба
    if (in.fire && !IsReloading() && !IsDashing()) {
        if (mag > 0) {
            if (fireCooldown <= 0.0f) Shoot(bullets, fx);
        } else if (reserve > 0) {
            StartReload();
        }
    }

    // --- Аварийный конденсатор: если патроны кончились совсем, медленно подзаряжается резерв,
    //     чтобы игра никогда не заходила в тупик.
    if (mag == 0 && reserve == 0) {
        emergencyTimer_ += dt;
        if (emergencyTimer_ >= 1.0f) {
            emergencyTimer_ -= 1.0f;
            reserve = std::min(reserveMax, reserve + 3);
        }
    } else {
        emergencyTimer_ = 0.0f;
    }
}

bool Player::TakeDamage(float dmg, Vector2 source, Effects& fx) {
    if (!alive || invulnTimer > 0.0f) return false;
    hp -= dmg;
    flashTimer = 0.18f;
    invulnTimer = 0.65f;
    Vector2 away = mu::Norm(pos - source);
    if (mu::LenSq(away) < 0.01f) away = Vector2{ 1.0f, 0.0f };
    vel += away * 380.0f;
    fx.shake.AddTrauma(0.5f);
    fx.particles.Burst(pos, 16, 60.0f, 260.0f, 2.0f, 4.5f, 0.25f, 0.6f, Color{ 255, 70, 90, 255 }, 3.0f);
    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
    }
    return true;
}

void Player::Heal(float amount) { hp = std::min(maxHp, hp + amount); }

void Player::AddAmmo(int amount) { reserve = std::min(reserveMax, reserve + amount); }

void Player::AddEssence(int amount) {
    essence += amount;
    while (essence >= essenceToNext) {
        essence -= essenceToNext;
        ++level;
        ++pendingUpgrades;
        essenceToNext = 10 + (level - 1) * 5;
    }
}

void Player::ApplyUpgrade(Upgrade u) {
    switch (u) {
        case Upgrade::Damage: damage *= 1.18f; break;
        case Upgrade::FireRate: fireDelay = std::max(0.05f, fireDelay * 0.88f); break;
        case Upgrade::MaxHealth: maxHp += 25.0f; Heal(25.0f); break;
        case Upgrade::MoveSpeed: moveSpeed *= 1.07f; break;
        case Upgrade::MagSize: magSize += 8; mag += 8; reserveMax += 30; reserve = std::min(reserveMax, reserve + 30); break;
        case Upgrade::Reload: reloadTime = std::max(0.4f, reloadTime * 0.8f); break;
    }
}

void Player::Draw(float time) const {
    if (!alive) return;
    DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y + radius * 0.8f), radius * 0.95f,
                radius * 0.45f, Fade(BLACK, 0.35f));

    const bool blink = invulnTimer > 0.0f && !IsDashing() && std::fmod(time * 18.0f, 2.0f) < 1.0f;
    const unsigned char alpha = blink ? 110 : 255;
    Color body = flashTimer > 0.0f ? WHITE : pal::kPlayer;
    body.a = alpha;
    Color dark = pal::kPlayerDark;
    dark.a = alpha;

    // Ствол
    const Vector2 tip = pos + aimDir * (radius + 15.0f);
    DrawLineEx(pos + aimDir * 4.0f, tip, 8.0f, Color{ 60, 70, 100, alpha });
    DrawLineEx(pos + aimDir * (radius - 2.0f), tip, 4.0f, body);

    // Корпус
    DrawCircleV(pos, radius + 3.0f, Fade(body, 0.18f));
    DrawCircleV(pos, radius, body);
    DrawCircleV(pos, radius - 4.0f, dark);
    DrawCircleV(pos + aimDir * 4.0f, 4.0f, body);

    // Кольцо перезарядки
    if (IsReloading()) {
        DrawRing(pos, radius + 6.0f, radius + 9.0f, -90.0f, -90.0f + 360.0f * ReloadProgress(), 32, pal::kAmmo);
    }
}
