#include "Enemy.h"

namespace {
constexpr float kTwoPi = 6.28318530718f;

// Плавное приближение скорости к желаемой.
inline void SteerVelocity(Vector2& vel, Vector2 desired, float rate, float dt) {
    vel += (desired - vel) * (1.0f - std::exp(-rate * dt));
}
}  // namespace

// ============================================================================
//  Фабрика и общие свойства
// ============================================================================
Enemy Enemy::Make(EnemyType type, Vector2 pos, int roomId, float hpScale) {
    Enemy e;
    e.type = type;
    e.pos = pos;
    e.roomId = roomId;
    switch (type) {
        case EnemyType::Chaser:
            e.radius = 12.0f; e.maxHp = 34.0f * hpScale; e.speed = 150.0f; e.contactDamage = 12.0f;
            break;
        case EnemyType::Shooter:
            e.radius = 11.0f; e.maxHp = 26.0f * hpScale; e.speed = 105.0f; e.contactDamage = 6.0f;
            break;
        case EnemyType::Tank:
            e.radius = 20.0f; e.maxHp = 150.0f * hpScale; e.speed = 68.0f; e.contactDamage = 20.0f;
            break;
        case EnemyType::Boss:
            e.radius = 46.0f; e.maxHp = 1600.0f; e.speed = 55.0f; e.contactDamage = 24.0f;
            break;
    }
    e.hp = e.maxHp;
    e.spawnDuration = (type == EnemyType::Boss) ? 2.2f : 0.7f + rng::Range(0.0f, 0.35f);
    e.spawnTimer = e.spawnDuration;
    e.attackTimer = rng::Range(0.5f, 1.3f);
    e.strafeDir = rng::Chance(0.5f) ? 1.0f : -1.0f;
    e.strafeTimer = rng::Range(0.8f, 2.0f);
    e.orbitDir = e.strafeDir;
    e.facing = rng::Range(0.0f, kTwoPi);
    return e;
}

int Enemy::ScoreValue() const {
    switch (type) {
        case EnemyType::Chaser: return 50;
        case EnemyType::Shooter: return 100;
        case EnemyType::Tank: return 250;
        case EnemyType::Boss: return 5000;
    }
    return 0;
}

int Enemy::EssenceDrop() const {
    switch (type) {
        case EnemyType::Chaser: return 1;
        case EnemyType::Shooter: return 2;
        case EnemyType::Tank: return 4;
        case EnemyType::Boss: return 30;
    }
    return 0;
}

Color Enemy::BaseColor() const {
    switch (type) {
        case EnemyType::Chaser: return pal::kChaser;
        case EnemyType::Shooter: return pal::kShooter;
        case EnemyType::Tank: return pal::kTank;
        case EnemyType::Boss: return pal::kBoss;
    }
    return WHITE;
}

bool Enemy::TakeDamage(float damage) {
    if (!alive || IsSpawning()) return false;
    hp -= damage;
    flashTimer = 0.09f;
    if (hp <= 0.0f) {
        hp = 0.0f;
        alive = false;
        return true;
    }
    return false;
}

// ============================================================================
//  ИИ
// ============================================================================
void Enemy::Update(float dt, AIContext& ctx) {
    if (!alive) return;
    flashTimer = std::max(0.0f, flashTimer - dt);
    animTime += dt;
    if (spawnTimer > 0.0f) {  // появление: враг неуязвим и бездействует
        spawnTimer -= dt;
        return;
    }
    switch (type) {
        case EnemyType::Chaser: UpdateChaser(dt, ctx); break;
        case EnemyType::Shooter: UpdateShooter(dt, ctx); break;
        case EnemyType::Tank: UpdateTank(dt, ctx); break;
        case EnemyType::Boss: UpdateBoss(dt, ctx); break;
    }
}

// Поиск пути: прямая видимость -> идём напрямую, иначе по полю расстояний (BFS).
Vector2 Enemy::ChaseDirection(const AIContext& ctx, bool los) const {
    const Vector2 direct = mu::Norm(ctx.player.pos - pos);
    if (los) return direct;
    const Vector2 flow = ctx.world.FlowDirection(pos);
    return mu::LenSq(flow) > 0.0f ? flow : direct;
}

void Enemy::Fire(AIContext& ctx, Vector2 dir, float bulletSpeed, float damage, float bulletRadius,
                 float life, Color color) const {
    Projectile b;
    b.pos = pos + dir * (radius + 6.0f);
    b.vel = dir * bulletSpeed;
    b.radius = bulletRadius;
    b.damage = damage;
    b.life = life;
    b.fromPlayer = false;
    b.color = color;
    ctx.bullets.push_back(b);
}

void Enemy::FireRing(AIContext& ctx, int count, float offset, float bulletSpeed, float damage) const {
    for (int i = 0; i < count; ++i) {
        const float a = offset + kTwoPi * static_cast<float>(i) / static_cast<float>(count);
        Fire(ctx, mu::FromAngle(a), bulletSpeed, damage, 6.0f, 5.5f, pal::kBoss);
    }
}

// ---------------------------------------------------------------- Chaser
// Преследует игрока, затем: замах (телеграф) -> рывок -> восстановление.
void Enemy::UpdateChaser(float dt, AIContext& ctx) {
    const Vector2 toP = ctx.player.pos - pos;
    const float dist = mu::Len(toP);
    const bool los = ctx.world.HasLineOfSight(pos, ctx.player.pos);
    stateTimer -= dt;
    attackCooldown -= dt;
    Vector2 desired{ 0.0f, 0.0f };

    switch (chaserState) {
        case ChaserState::Chase:
            desired = ChaseDirection(ctx, los) * speed;
            if (ctx.player.alive && los && dist < 170.0f && attackCooldown <= 0.0f) {
                chaserState = ChaserState::Windup;
                stateTimer = 0.38f;
            }
            break;
        case ChaserState::Windup:
            lungeDir = mu::Norm(toP);
            if (stateTimer <= 0.0f) {
                chaserState = ChaserState::Lunge;
                stateTimer = 0.30f;
            }
            break;
        case ChaserState::Lunge:
            vel = lungeDir * speed * 3.6f;
            if (stateTimer <= 0.0f) {
                chaserState = ChaserState::Recover;
                stateTimer = 0.55f;
                attackCooldown = 1.5f;
            }
            break;
        case ChaserState::Recover:
            if (stateTimer <= 0.0f) chaserState = ChaserState::Chase;
            break;
    }
    if (chaserState != ChaserState::Lunge) SteerVelocity(vel, desired, 9.0f, dt);
    ctx.world.MoveCircle(pos, vel * dt, radius);

    if (chaserState == ChaserState::Windup || chaserState == ChaserState::Lunge) facing = mu::AngleOf(lungeDir);
    else if (mu::LenSq(vel) > 100.0f) facing = mu::AngleOf(vel);
}

// --------------------------------------------------------------- Shooter
// Держит дистанцию, стрейфит, стреляет очередями по 3 выстрела, затем "перезаряжается".
void Enemy::UpdateShooter(float dt, AIContext& ctx) {
    const Vector2 toP = ctx.player.pos - pos;
    const float dist = mu::Len(toP);
    const bool los = ctx.world.HasLineOfSight(pos, ctx.player.pos);
    const Vector2 toDir = mu::Norm(toP);

    strafeTimer -= dt;
    if (strafeTimer <= 0.0f) {
        strafeDir = -strafeDir;
        strafeTimer = rng::Range(1.2f, 2.8f);
    }

    constexpr float kNear = 230.0f;
    constexpr float kFar = 340.0f;
    Vector2 dir{ 0.0f, 0.0f };
    if (!los) dir = ChaseDirection(ctx, false);
    else if (dist < kNear) dir = (-toDir) + mu::Perp(toDir) * (strafeDir * 0.6f);
    else if (dist > kFar) dir = toDir;
    else dir = mu::Perp(toDir) * strafeDir;
    SteerVelocity(vel, mu::Norm(dir) * speed, 7.0f, dt);
    ctx.world.MoveCircle(pos, vel * dt, radius);
    facing = mu::AngleOf(toP);

    if (reloadTimer > 0.0f) {
        reloadTimer -= dt;
    } else if (los && dist < 540.0f && ctx.player.alive) {
        attackTimer -= dt;
        if (attackTimer <= 0.0f) {
            const Vector2 lead = ctx.player.pos + ctx.player.vel * (dist / 380.0f) * 0.4f;
            const float ang = mu::AngleOf(lead - pos) + rng::Range(-0.06f, 0.06f);
            Fire(ctx, mu::FromAngle(ang), 380.0f, 8.0f, 5.0f, 2.4f, pal::kShooter);
            ctx.fx.Muzzle(pos + mu::FromAngle(ang) * (radius + 8.0f), mu::FromAngle(ang), pal::kShooter);
            --burstLeft;
            attackTimer = 0.2f;
            if (burstLeft <= 0) {
                burstLeft = 3;
                reloadTimer = 1.8f;
                attackTimer = 0.35f;
            }
        }
    } else {
        attackTimer = std::max(attackTimer, 0.25f);
    }
}

// ------------------------------------------------------------------ Tank
// Медленно наступает и стреляет веером из 5 снарядов.
void Enemy::UpdateTank(float dt, AIContext& ctx) {
    const Vector2 toP = ctx.player.pos - pos;
    const float dist = mu::Len(toP);
    const bool los = ctx.world.HasLineOfSight(pos, ctx.player.pos);
    attackTimer -= dt;

    Vector2 desired{ 0.0f, 0.0f };
    if (!los) desired = ChaseDirection(ctx, false) * speed;
    else if (dist > 190.0f) desired = mu::Norm(toP) * speed;
    else if (dist < 120.0f) desired = -mu::Norm(toP) * speed * 0.6f;
    SteerVelocity(vel, desired, 5.0f, dt);
    ctx.world.MoveCircle(pos, vel * dt, radius);
    facing = mu::AngleOf(toP);

    if (los && dist < 420.0f && ctx.player.alive) {
        if (attackTimer <= 0.0f) {
            attackTimer = 2.6f;
            const float base = mu::AngleOf(toP);
            for (int i = -2; i <= 2; ++i) {
                Fire(ctx, mu::FromAngle(base + static_cast<float>(i) * 0.2f), 300.0f, 7.0f, 5.0f, 2.0f, pal::kTank);
            }
            ctx.fx.Muzzle(pos + mu::FromAngle(base) * (radius + 8.0f), mu::FromAngle(base), pal::kTank);
            ctx.fx.shake.AddTrauma(0.05f);
        }
    } else {
        attackTimer = std::max(attackTimer, 0.0f);
    }
}

// ------------------------------------------------------------------ Boss
// Главное Ядро: 3 фазы. Спирали пуль, прицельные залпы, кольца, вызов подкреплений.
void Enemy::UpdateBoss(float dt, AIContext& ctx) {
    const float ratio = hp / maxHp;
    const int newPhase = ratio > 0.66f ? 1 : (ratio > 0.33f ? 2 : 3);
    if (newPhase != phase) {
        phase = newPhase;
        pendingPhase = newPhase;
        ringTimer = 0.4f;
        summonTimer = 1.5f;
        ctx.fx.shake.AddTrauma(0.8f);
        ctx.fx.Explosion(pos, 1.8f, BaseColor());
    }

    const Vector2 toP = ctx.player.pos - pos;
    const float dist = mu::Len(toP);
    const Vector2 toDir = mu::Norm(toP);
    facing = mu::AngleOf(toP);

    // Движение: держит среднюю дистанцию и кружит вокруг игрока.
    orbitTimer -= dt;
    if (orbitTimer <= 0.0f) {
        orbitDir = -orbitDir;
        orbitTimer = rng::Range(2.5f, 4.5f);
    }
    const float spd = phase == 1 ? 55.0f : (phase == 2 ? 75.0f : 100.0f);
    Vector2 dir{ 0.0f, 0.0f };
    if (dist > 340.0f) dir = toDir;
    else if (dist < 220.0f) dir = -toDir + mu::Perp(toDir) * (orbitDir * 0.5f);
    else dir = mu::Perp(toDir) * orbitDir;
    SteerVelocity(vel, mu::Norm(dir) * spd, 3.0f, dt);
    ctx.world.MoveCircle(pos, vel * dt, radius);

    if (!ctx.player.alive) return;

    // Спираль
    flipTimer -= dt;
    if (phase == 3 && flipTimer <= 0.0f) {
        spiralDir = -spiralDir;
        flipTimer = 3.0f;
    }
    spiralTimer -= dt;
    if (spiralTimer <= 0.0f) {
        const int arms = phase + 1;
        spiralTimer = phase == 1 ? 0.16f : (phase == 2 ? 0.20f : 0.22f);
        for (int a = 0; a < arms; ++a) {
            const float ang = spiralAngle + kTwoPi * static_cast<float>(a) / static_cast<float>(arms);
            Fire(ctx, mu::FromAngle(ang), 230.0f, 9.0f, 6.0f, 5.0f, pal::kBoss);
        }
        spiralAngle += spiralDir * 0.21f;
    }

    // Прицельный залп из трёх пуль
    aimedTimer -= dt;
    if (aimedTimer <= 0.0f) {
        aimedTimer = phase == 1 ? 3.2f : 2.6f;
        for (int i = -1; i <= 1; ++i) {
            Fire(ctx, mu::FromAngle(facing + static_cast<float>(i) * 0.16f), 380.0f, 12.0f, 7.0f, 3.0f,
                 Color{ 255, 150, 220, 255 });
        }
        ctx.fx.shake.AddTrauma(0.1f);
    }

    // Кольцо пуль (со 2-й фазы)
    if (phase >= 2) {
        ringTimer -= dt;
        if (ringTimer <= 0.0f) {
            ringTimer = phase == 2 ? 3.8f : 2.8f;
            ringOffset += 0.13f;
            FireRing(ctx, 16 + phase * 2, ringOffset, 200.0f, 10.0f);
            ctx.fx.shake.AddTrauma(0.2f);
            ctx.fx.Shock(pos, 120.0f, 0.5f, pal::kBoss);
        }

        // Вызов подкреплений
        summonTimer -= dt;
        if (summonTimer <= 0.0f) {
            summonTimer = phase == 2 ? 10.0f : 8.0f;
            for (int i = 0; i < 2; ++i) {
                ctx.spawns.push_back(EnemySpawnRequest{ phase == 2 ? EnemyType::Chaser : EnemyType::Shooter, roomId });
            }
        }
    }
}

// ============================================================================
//  Отрисовка
// ============================================================================
void Enemy::Draw(float time) const {
    if (!alive) return;
    const Color base = BaseColor();

    // Появление: пульсирующий портал вместо врага.
    if (spawnTimer > 0.0f) {
        const float t = 1.0f - spawnTimer / spawnDuration;
        DrawCircleV(pos, radius * (0.4f + 0.6f * t), Fade(base, 0.25f + 0.35f * t));
        const float rr = radius * (2.0f - t);
        DrawRing(pos, rr, rr + 2.0f, 0.0f, 360.0f, 32, Fade(base, 0.8f));
        return;
    }

    DrawEllipse(static_cast<int>(pos.x), static_cast<int>(pos.y + radius * 0.8f), radius * 0.95f,
                radius * 0.45f, Fade(BLACK, 0.35f));
    Color c = flashTimer > 0.0f ? WHITE : base;
    const float rot = facing * RAD2DEG;
    const Vector2 fwd = mu::FromAngle(facing);

    switch (type) {
        case EnemyType::Chaser: {
            if (chaserState == ChaserState::Windup) {
                DrawLineEx(pos, pos + lungeDir * 170.0f, 2.0f, Fade(base, 0.5f));
                if (std::fmod(animTime * 20.0f, 2.0f) < 1.0f) c = WHITE;
            }
            DrawCircleV(pos, radius + 6.0f, Fade(c, 0.14f));
            DrawPoly(pos, 3, radius + 5.0f, rot, c);
            DrawPoly(pos, 3, radius - 2.0f, rot, Color{ 40, 8, 16, 255 });
            DrawCircleV(pos + fwd * 4.0f, 3.0f, c);
            break;
        }
        case EnemyType::Shooter: {
            const Color dim = reloadTimer > 0.0f ? Fade(c, 0.6f) : c;
            DrawLineEx(pos, pos + fwd * (radius + 11.0f), 5.0f, dim);
            DrawCircleV(pos, radius + 6.0f, Fade(c, 0.12f));
            DrawPoly(pos, 4, radius + 4.0f, rot + 45.0f, dim);
            DrawPoly(pos, 4, radius - 3.0f, rot + 45.0f, Color{ 50, 30, 8, 255 });
            DrawCircleV(pos + fwd * 3.0f, 3.0f, dim);
            break;
        }
        case EnemyType::Tank: {
            if (attackTimer < 0.45f && attackTimer > 0.0f) c = mu::LerpColor(c, WHITE, 0.7f);
            DrawLineEx(pos, pos + fwd * (radius + 14.0f), 9.0f, c);
            DrawCircleV(pos, radius + 8.0f, Fade(c, 0.14f));
            DrawPoly(pos, 6, radius + 2.0f, rot, c);
            DrawPoly(pos, 6, radius - 5.0f, rot, Color{ 36, 18, 60, 255 });
            DrawPoly(pos, 6, radius - 11.0f, rot + 30.0f, c);
            break;
        }
        case EnemyType::Boss: {
            const float pulse = 0.5f + 0.5f * std::sin(time * 4.0f);
            Color core = phase == 1 ? pal::kBoss : (phase == 2 ? Color{ 255, 140, 60, 255 } : Color{ 255, 70, 70, 255 });
            if (flashTimer > 0.0f) core = WHITE;
            DrawCircleV(pos, radius + 20.0f + 6.0f * pulse, Fade(core, 0.10f));
            DrawCircleV(pos, radius + 9.0f, Fade(core, 0.20f));
            DrawCircleV(pos, radius, Color{ 18, 12, 30, 255 });
            for (int i = 0; i < 3; ++i) {
                const float rotRing = time * (40.0f * static_cast<float>(i + 1)) * ((i % 2) ? -1.0f : 1.0f);
                const float inner = radius - 9.0f - static_cast<float>(i) * 9.0f;
                DrawRing(pos, inner, inner + 3.0f, rotRing, rotRing + 250.0f - static_cast<float>(i) * 40.0f, 48, Fade(core, 0.9f));
            }
            for (int i = 0; i < 8; ++i) {
                const float a = time * 0.6f + kTwoPi * static_cast<float>(i) / 8.0f;
                DrawLineEx(pos + mu::FromAngle(a) * (radius + 2.0f), pos + mu::FromAngle(a) * (radius + 14.0f), 4.0f, core);
            }
            DrawCircleV(pos, radius * 0.38f + 3.0f * pulse, core);
            DrawCircleV(pos + fwd * 8.0f, radius * 0.16f, WHITE);
            break;
        }
    }
}

void Enemy::DrawHealthBar() const {
    if (!alive || type == EnemyType::Boss || IsSpawning() || hp >= maxHp) return;
    const int w = static_cast<int>(radius * 2.0f + 10.0f);
    const int x = static_cast<int>(pos.x) - w / 2;
    const int y = static_cast<int>(pos.y - radius - 14.0f);
    DrawRectangle(x, y, w, 4, Color{ 20, 20, 30, 210 });
    DrawRectangle(x, y, static_cast<int>(static_cast<float>(w) * hp / maxHp), 4, BaseColor());
}
