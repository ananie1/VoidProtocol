#pragma once
// Игрок: движение, рывок, стрельба, перезарядка, прокачка.
#include "Common.h"
#include "Effects.h"
#include "Projectile.h"
#include "World.h"

#include <vector>

struct PlayerInput {
    Vector2 move{ 0.0f, 0.0f };
    Vector2 aimWorld{ 0.0f, 0.0f };
    bool fire = false;
    bool reload = false;
    bool dash = false;
};

enum class Upgrade { Damage, FireRate, MaxHealth, MoveSpeed, MagSize, Reload };
inline constexpr int kUpgradeCount = 6;
const char* UpgradeName(Upgrade u);
const char* UpgradeDesc(Upgrade u);
Color UpgradeColor(Upgrade u);

class Player {
public:
    void Reset(Vector2 spawn);
    void Update(float dt, const PlayerInput& in, const World& world, std::vector<Projectile>& bullets,
                Effects& fx);
    // Возвращает true, если урон реально нанесён (нет кадров неуязвимости).
    bool TakeDamage(float damage, Vector2 source, Effects& fx);
    void Heal(float amount);
    void AddAmmo(int amount);
    void AddEssence(int amount);
    void ApplyUpgrade(Upgrade u);
    void Draw(float time) const;

    bool IsReloading() const { return reloadTimer > 0.0f; }
    bool IsDashing() const { return dashTimer > 0.0f; }
    float ReloadProgress() const { return IsReloading() ? 1.0f - reloadTimer / reloadTime : 0.0f; }
    bool NeedsHealth() const { return hp < maxHp - 0.5f; }
    bool NeedsAmmo() const { return reserve < reserveMax; }
    int TotalAmmo() const { return mag + reserve; }

    // Состояние
    Vector2 pos{ 0.0f, 0.0f };
    Vector2 vel{ 0.0f, 0.0f };
    Vector2 aimDir{ 1.0f, 0.0f };
    float radius = 12.0f;
    bool alive = true;

    // Здоровье
    float hp = 100.0f;
    float maxHp = 100.0f;
    float flashTimer = 0.0f;    // белая вспышка при уроне
    float invulnTimer = 0.0f;   // кадры неуязвимости

    // Оружие
    int mag = 30;
    int magSize = 30;
    int reserve = 120;
    int reserveMax = 240;
    float damage = 14.0f;
    float fireDelay = 0.11f;
    float fireCooldown = 0.0f;
    float bulletSpeed = 820.0f;
    float reloadTime = 1.5f;
    float reloadTimer = 0.0f;

    // Движение
    float moveSpeed = 250.0f;
    float accel = 14.0f;
    float dashSpeed = 760.0f;
    float dashTime = 0.16f;
    float dashTimer = 0.0f;
    float dashCooldown = 0.0f;
    float dashCooldownMax = 1.1f;
    Vector2 dashDir{ 1.0f, 0.0f };

    // Прогрессия
    int level = 1;
    int essence = 0;
    int essenceToNext = 10;
    int pendingUpgrades = 0;

private:
    void StartReload();
    void Shoot(std::vector<Projectile>& bullets, Effects& fx);

    float trailTimer_ = 0.0f;
    float emergencyTimer_ = 0.0f;
};
