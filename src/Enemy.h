#pragma once
// Враги: Chaser (таран), Shooter (стрелок), Tank (тяжёлый дроид), Boss (Главное Ядро).
#include "Common.h"
#include "Effects.h"
#include "Player.h"
#include "Projectile.h"
#include "World.h"

#include <vector>

enum class EnemyType { Chaser, Shooter, Tank, Boss };
enum class ChaserState { Chase, Windup, Lunge, Recover };

struct EnemySpawnRequest {
    EnemyType type = EnemyType::Chaser;
    int roomId = -1;
};

// Всё, что нужно ИИ для работы: мир, игрок, пули, эффекты и очередь вызова подкреплений.
struct AIContext {
    const World& world;
    Player& player;
    std::vector<Projectile>& bullets;
    Effects& fx;
    std::vector<EnemySpawnRequest>& spawns;
};

class Enemy {
public:
    static Enemy Make(EnemyType type, Vector2 pos, int roomId, float hpScale);

    void Update(float dt, AIContext& ctx);
    // true - если враг убит этим ударом.
    bool TakeDamage(float damage);
    void Draw(float time) const;
    void DrawHealthBar() const;

    bool IsSpawning() const { return spawnTimer > 0.0f; }
    bool IsLunging() const { return type == EnemyType::Chaser && chaserState == ChaserState::Lunge; }
    float ContactDamage() const { return IsLunging() ? contactDamage * 1.6f : contactDamage; }
    int ScoreValue() const;
    int EssenceDrop() const;
    Color BaseColor() const;
    // Возвращает номер новой фазы босса (или 0, если фаза не менялась).
    int ConsumePhaseChange() {
        const int p = pendingPhase;
        pendingPhase = 0;
        return p;
    }

    EnemyType type = EnemyType::Chaser;
    Vector2 pos{ 0.0f, 0.0f };
    Vector2 vel{ 0.0f, 0.0f };
    int roomId = -1;
    bool alive = true;

    float radius = 12.0f;
    float hp = 1.0f;
    float maxHp = 1.0f;
    float speed = 100.0f;
    float contactDamage = 10.0f;

    float flashTimer = 0.0f;
    float spawnTimer = 0.0f;
    float spawnDuration = 0.7f;
    float animTime = 0.0f;
    float facing = 0.0f;

    // ИИ
    float attackTimer = 1.0f;
    float reloadTimer = 0.0f;
    float stateTimer = 0.0f;
    float attackCooldown = 0.0f;
    float strafeTimer = 1.0f;
    float strafeDir = 1.0f;
    int burstLeft = 3;
    ChaserState chaserState = ChaserState::Chase;
    Vector2 lungeDir{ 1.0f, 0.0f };

    // Босс
    int phase = 1;
    int pendingPhase = 0;
    float spiralAngle = 0.0f;
    float spiralDir = 1.0f;
    float spiralTimer = 0.8f;
    float aimedTimer = 2.5f;
    float ringTimer = 3.0f;
    float ringOffset = 0.0f;
    float summonTimer = 7.0f;
    float orbitDir = 1.0f;
    float orbitTimer = 3.0f;
    float flipTimer = 3.5f;

private:
    void UpdateChaser(float dt, AIContext& ctx);
    void UpdateShooter(float dt, AIContext& ctx);
    void UpdateTank(float dt, AIContext& ctx);
    void UpdateBoss(float dt, AIContext& ctx);
    Vector2 ChaseDirection(const AIContext& ctx, bool los) const;
    void Fire(AIContext& ctx, Vector2 dir, float bulletSpeed, float damage, float bulletRadius,
              float life, Color color) const;
    void FireRing(AIContext& ctx, int count, float offset, float bulletSpeed, float damage) const;
};
