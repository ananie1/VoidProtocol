#include "Game.h"

#include <algorithm>
#include <cstdlib>
#include <numeric>

namespace {
constexpr const char* kSaveFile = "void_protocol.sav";

Vector2 ScreenCenter() {
    return Vector2{ static_cast<float>(cfg::kScreenW) * 0.5f, static_cast<float>(cfg::kScreenH) * 0.5f };
}
}  // namespace

// ============================================================================
//  Создание, главный цикл, состояния
// ============================================================================
Game::Game()
    : menuButtons_({ "START RUN", "QUIT" }, 380.0f),
      pauseButtons_({ "RESUME", "RESTART RUN", "MAIN MENU" }, 260.0f),
      overButtons_({ "TRY AGAIN", "MAIN MENU" }, 520.0f),
      victoryButtons_({ "PLAY AGAIN", "MAIN MENU" }, 520.0f) {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(cfg::kScreenW, cfg::kScreenH, "Void Protocol");
    SetExitKey(KEY_NULL);  // ESC используется для паузы, а не для выхода
    SetTargetFPS(144);

    camera_.zoom = 1.0f;
    camera_.offset = ScreenCenter();

    for (int i = 0; i < 150; ++i) {
        Star s;
        s.pos = Vector2{ rng::Range(0.0f, static_cast<float>(cfg::kScreenW)), rng::Range(0.0f, static_cast<float>(cfg::kScreenH)) };
        s.speed = rng::Range(8.0f, 90.0f);
        s.size = rng::Range(0.8f, 2.4f);
        stars_.push_back(s);
    }
    LoadBest();
    SetState(GameState::Menu);
}

Game::~Game() { CloseWindow(); }

void Game::Run() {
    while (!quit_ && !WindowShouldClose()) {
        const float dt = std::min(GetFrameTime(), 0.05f);  // защита от больших скачков времени
        time_ += dt;
        Update(dt);
        BeginDrawing();
        Draw();
        EndDrawing();
    }
}

void Game::SetState(GameState s) {
    state_ = s;
    if (s == GameState::Playing) HideCursor(); else ShowCursor();
    switch (s) {
        case GameState::Menu: menuButtons_.Reset(); break;
        case GameState::Paused: pauseButtons_.Reset(); break;
        case GameState::GameOver: overButtons_.Reset(); break;
        case GameState::Victory: victoryButtons_.Reset(); break;
        default: break;
    }
}

void Game::Update(float dt) {
    switch (state_) {
        case GameState::Menu: UpdateMenu(dt); break;
        case GameState::Playing: UpdatePlaying(dt); break;
        case GameState::Paused: UpdatePaused(); break;
        case GameState::Upgrade: UpdateUpgrade(); break;
        case GameState::GameOver:
        case GameState::Victory:
            UpdateStars(dt);
            UpdateEndScreen();
            break;
    }
}

void Game::Draw() {
    ClearBackground(pal::kBackground);
    switch (state_) {
        case GameState::Menu: DrawMenu(); break;
        case GameState::Playing: DrawPlaying(true, true); break;
        case GameState::Paused:
            DrawPlaying(true, false);
            DrawPauseOverlay();
            break;
        case GameState::Upgrade:
            DrawPlaying(true, false);
            DrawUpgradeOverlay();
            break;
        case GameState::GameOver:
            DrawPlaying(false, false);
            DrawEndScreen(false);
            break;
        case GameState::Victory:
            DrawPlaying(false, false);
            DrawEndScreen(true);
            break;
    }
}

void Game::StartRun() {
    world_.Generate();
    player_.Reset(world_.StartPosition());
    enemies_.clear();
    bullets_.clear();
    pickups_.clear();
    spawnQueue_.clear();
    fx_.Reset();

    runTime_ = 0.0f;
    score_ = 0;
    kills_ = 0;
    roomsCleared_ = 0;
    currentRoom_ = world_.StartRoomId();
    activeRoom_ = -1;
    waveDelay_ = 0.0f;
    flowTimer_ = 0.0f;
    timeScale_ = 1.0f;
    hurtFlash_ = 0.0f;
    runEnd_ = RunEnd::None;
    endTimer_ = 0.0f;
    bossBlastTimer_ = 0.0f;
    bannerTimer_ = 0.0f;
    newBest_ = false;
    fireLock_ = 0.25f;

    camPos_ = player_.pos;
    UpdateCamera(10.0f);
    world_.UpdateFlowField(player_.pos);
    ShowBanner("SECTOR BREACHED - PURGE THE STATION", pal::kUi, 3.0f);
    SetState(GameState::Playing);
}

void Game::FinishRun(bool victory) {
    if (victory) {
        const int timeBonus = std::max(0, 900 - static_cast<int>(runTime_)) * 10;
        score_ += timeBonus;
    }
    finalScore_ = score_;
    newBest_ = finalScore_ > bestScore_;
    if (newBest_) {
        bestScore_ = finalScore_;
        SaveBest();
    }
    SetState(victory ? GameState::Victory : GameState::GameOver);
}

void Game::LoadBest() {
    char* text = LoadFileText(kSaveFile);
    if (text != nullptr) {
        bestScore_ = std::max(0, std::atoi(text));
        UnloadFileText(text);
    }
}

void Game::SaveBest() const {
    std::string s = std::to_string(bestScore_);
    SaveFileText(kSaveFile, s.data());
}

void Game::ShowBanner(const std::string& text, Color color, float duration) {
    bannerText_ = text;
    bannerColor_ = color;
    bannerTimer_ = duration;
    bannerTotal_ = duration;
}

// ============================================================================
//  Меню и вспомогательные экраны (update)
// ============================================================================
void Game::UpdateStars(float dt) {
    for (Star& s : stars_) {
        s.pos.x -= s.speed * dt;
        if (s.pos.x < 0.0f) {
            s.pos.x += static_cast<float>(cfg::kScreenW);
            s.pos.y = rng::Range(0.0f, static_cast<float>(cfg::kScreenH));
        }
    }
}

void Game::UpdateMenu(float dt) {
    UpdateStars(dt);
    const int c = menuButtons_.Update();
    if (c == 0) StartRun();
    else if (c == 1) quit_ = true;
}

void Game::UpdatePaused() {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
        SetState(GameState::Playing);
        fireLock_ = 0.2f;
        return;
    }
    const int c = pauseButtons_.Update();
    if (c == 0) {
        SetState(GameState::Playing);
        fireLock_ = 0.2f;
    } else if (c == 1) {
        StartRun();
    } else if (c == 2) {
        SetState(GameState::Menu);
    }
}

void Game::UpdateEndScreen() {
    ui::ButtonList& list = (state_ == GameState::Victory) ? victoryButtons_ : overButtons_;
    const int c = list.Update();
    if (c == 0) StartRun();
    else if (c == 1) SetState(GameState::Menu);
}

void Game::RollUpgrades() {
    std::array<int, kUpgradeCount> idx{};
    std::iota(idx.begin(), idx.end(), 0);
    std::shuffle(idx.begin(), idx.end(), rng::Engine());
    for (std::size_t i = 0; i < choices_.size(); ++i) choices_[i] = static_cast<Upgrade>(idx[i]);
    upgradeSelected_ = 0;
}

void Game::UpdateUpgrade() {
    int pick = -1;
    if (IsKeyPressed(KEY_ONE) || IsKeyPressed(KEY_KP_1)) pick = 0;
    if (IsKeyPressed(KEY_TWO) || IsKeyPressed(KEY_KP_2)) pick = 1;
    if (IsKeyPressed(KEY_THREE) || IsKeyPressed(KEY_KP_3)) pick = 2;
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) upgradeSelected_ = (upgradeSelected_ + 2) % 3;
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) upgradeSelected_ = (upgradeSelected_ + 1) % 3;

    const Vector2 mouse = GetMousePosition();
    const Vector2 delta = GetMouseDelta();
    for (int i = 0; i < 3; ++i) {
        if (!CheckCollisionPointRec(mouse, ui::UpgradeCardRect(i))) continue;
        if (delta.x != 0.0f || delta.y != 0.0f) upgradeSelected_ = i;
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) pick = i;
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) pick = upgradeSelected_;

    if (pick >= 0) {
        player_.ApplyUpgrade(choices_[static_cast<std::size_t>(pick)]);
        fx_.Sparkle(player_.pos, UpgradeColor(choices_[static_cast<std::size_t>(pick)]));
        --player_.pendingUpgrades;
        if (player_.pendingUpgrades > 0) {
            RollUpgrades();
        } else {
            SetState(GameState::Playing);
            fireLock_ = 0.25f;
        }
    }
}

// ============================================================================
//  Игровой процесс
// ============================================================================
PlayerInput Game::PollInput() const {
    PlayerInput in;
    Vector2 m{ 0.0f, 0.0f };
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) m.y -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) m.y += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) m.x -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) m.x += 1.0f;
    in.move = m;

    Camera2D c = camera_;
    c.offset = ScreenCenter();  // прицел не должен дрожать вместе с экраном
    in.aimWorld = GetScreenToWorld2D(GetMousePosition(), c);
    in.fire = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && fireLock_ <= 0.0f;
    in.reload = IsKeyPressed(KEY_R);
    in.dash = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_LEFT_SHIFT);
    return in;
}

void Game::UpdatePlaying(float dt) {
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P) || !IsWindowFocused()) {
        SetState(GameState::Paused);
        return;
    }
    fireLock_ = std::max(0.0f, fireLock_ - dt);
    PlayerInput in = PollInput();
    if (runEnd_ == RunEnd::Won) in.fire = false;

    // Симуляция идёт мелкими шагами (<= 1/120 c): стабильные коллизии при любом FPS.
    const float scaled = dt * timeScale_;
    const int steps = std::max(1, static_cast<int>(std::ceil(scaled / cfg::kSimStep)));
    const float h = scaled / static_cast<float>(steps);
    for (int i = 0; i < steps && state_ == GameState::Playing; ++i) {
        SimStep(h, in);
        in.reload = false;  // одноразовые нажатия обрабатываются только в первом шаге
        in.dash = false;
    }
    if (state_ != GameState::Playing) return;

    if (runEnd_ == RunEnd::None) runTime_ += scaled;
    fx_.Update(scaled);
    hurtFlash_ = std::max(0.0f, hurtFlash_ - dt * 2.5f);
    bannerTimer_ = std::max(0.0f, bannerTimer_ - dt);
    UpdateCamera(dt);

    if (runEnd_ == RunEnd::Dead) {
        endTimer_ -= dt;
        if (endTimer_ <= 0.0f) FinishRun(false);
    } else if (runEnd_ == RunEnd::Won) {
        bossBlastTimer_ -= dt;
        if (bossBlastTimer_ <= 0.0f) {  // цепочка взрывов на месте босса
            bossBlastTimer_ = 0.22f;
            fx_.Explosion(bossDeathPos_ + rng::Direction() * rng::Range(0.0f, 70.0f), rng::Range(0.9f, 1.8f), pal::kBoss);
            fx_.shake.AddTrauma(0.35f);
        }
        endTimer_ -= dt;
        if (endTimer_ <= 0.0f) FinishRun(true);
    }
}

void Game::SimStep(float h, const PlayerInput& in) {
    if (player_.alive) player_.Update(h, in, world_, bullets_, fx_);
    UpdateRoomLogic(h);

    flowTimer_ -= h;
    if (flowTimer_ <= 0.0f) {
        world_.UpdateFlowField(player_.pos);
        flowTimer_ = 0.12f;
    }

    UpdateEnemies(h);
    ResolveEnemyCollisions();
    UpdateBullets(h);
    UpdatePickups(h);

    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return !e.alive; }), enemies_.end());
    bullets_.erase(std::remove_if(bullets_.begin(), bullets_.end(), [](const Projectile& b) { return !b.alive; }), bullets_.end());
    pickups_.erase(std::remove_if(pickups_.begin(), pickups_.end(), [](const Pickup& p) { return !p.alive; }), pickups_.end());

    if (!player_.alive && runEnd_ == RunEnd::None) {
        runEnd_ = RunEnd::Dead;
        endTimer_ = 1.6f;
        timeScale_ = 0.3f;
        fx_.Explosion(player_.pos, 2.0f, pal::kPlayer);
        fx_.shake.AddTrauma(1.0f);
        ShowBanner("SIGNAL LOST", Color{ 255, 70, 90, 255 }, 3.0f);
    }
    if (player_.alive && runEnd_ == RunEnd::None && player_.pendingUpgrades > 0) {
        RollUpgrades();
        SetState(GameState::Upgrade);
    }
}

// ----------------------------------------------------------- Комнаты и волны
void Game::UpdateRoomLogic(float h) {
    if (!player_.alive || runEnd_ == RunEnd::Won) return;

    const int inner = world_.RoomAtInner(player_.pos);
    if (inner >= 0) {
        currentRoom_ = inner;
        Room& r = world_.GetRoom(inner);
        r.visited = true;
        if (r.state == RoomState::Idle && r.type != RoomType::Start && activeRoom_ < 0) EnterRoom(r);
    }
    if (activeRoom_ < 0) return;

    Room& room = world_.GetRoom(activeRoom_);
    if (waveDelay_ > 0.0f) {
        waveDelay_ -= h;
        if (waveDelay_ <= 0.0f) SpawnWave(room);
        return;
    }
    int alive = 0;
    for (const Enemy& e : enemies_) {
        if (e.alive && e.roomId == room.id) ++alive;
    }
    if (alive > 0) return;
    if (room.wavesSpawned < room.wavesTotal) {
        waveDelay_ = 1.2f;
        ShowBanner("WAVE CLEARED", pal::kHealth, 1.2f);
    } else {
        ClearRoom(room);
    }
}

void Game::EnterRoom(Room& room) {
    room.state = RoomState::Active;
    activeRoom_ = room.id;
    world_.LockRoom(room.id);
    waveDelay_ = room.type == RoomType::Boss ? 1.6f : 0.9f;
    fx_.shake.AddTrauma(0.25f);
    if (room.type == RoomType::Boss) ShowBanner("SEALED - THE CORE AWAKENS", pal::kBoss, 2.5f);
    else ShowBanner("HOSTILES DETECTED - DOORS LOCKED", Color{ 255, 110, 110, 255 }, 2.2f);
}

EnemyType Game::PickEnemyType(int depth) const {
    const int wChaser = 6;
    const int wShooter = 3 + std::min(depth, 4);
    const int wTank = depth >= 2 ? std::min(4, 1 + depth / 2) : 0;
    int roll = rng::Int(1, wChaser + wShooter + wTank);
    if (roll <= wChaser) return EnemyType::Chaser;
    roll -= wChaser;
    if (roll <= wShooter) return EnemyType::Shooter;
    return EnemyType::Tank;
}

void Game::SpawnWave(Room& room) {
    ++room.wavesSpawned;
    if (room.type == RoomType::Boss) {
        enemies_.push_back(Enemy::Make(EnemyType::Boss, room.Center(), room.id, 1.0f));
        fx_.SpawnPortal(room.Center(), pal::kBoss);
        fx_.shake.AddTrauma(0.6f);
        ShowBanner("WARNING - MAIN CORE ONLINE", pal::kBoss, 3.0f);
        return;
    }
    const int count = std::min(10, 3 + room.depth / 2 + (room.wavesSpawned - 1));
    const float hpScale = 1.0f + 0.10f * static_cast<float>(room.depth);
    for (int i = 0; i < count; ++i) {
        const EnemyType t = PickEnemyType(room.depth);
        const Vector2 p = world_.RandomSpawnPoint(room, player_.pos, 220.0f, 22.0f);
        enemies_.push_back(Enemy::Make(t, p, room.id, hpScale));
        fx_.SpawnPortal(p, enemies_.back().BaseColor());
    }
    ShowBanner(TextFormat("WAVE %d / %d", room.wavesSpawned, room.wavesTotal), Color{ 255, 150, 90, 255 }, 1.8f);
}

void Game::ClearRoom(Room& room) {
    room.state = RoomState::Cleared;
    world_.UnlockRoom(room.id);
    activeRoom_ = -1;
    if (room.type == RoomType::Boss) return;

    ++roomsCleared_;
    score_ += 100 * std::max(1, room.depth);
    ShowBanner("ROOM CLEARED - DOORS UNLOCKED", pal::kHealth, 2.4f);
    const Vector2 c = room.Center();
    SpawnPickup(PickupType::Health, c + Vector2{ -30.0f, 0.0f }, 30);
    SpawnPickup(PickupType::Ammo, c + Vector2{ 30.0f, 0.0f }, 30);
    for (int i = 0; i < 4; ++i) SpawnPickup(PickupType::Essence, c, 1);
    fx_.Shock(c, 160.0f, 0.7f, pal::kHealth);
}

// ----------------------------------------------------------- Враги и бой
void Game::UpdateEnemies(float h) {
    AIContext ctx{ world_, player_, bullets_, fx_, spawnQueue_ };
    for (Enemy& e : enemies_) {
        if (!e.alive) continue;
        e.Update(h, ctx);
        if (e.type == EnemyType::Boss) {
            const int ph = e.ConsumePhaseChange();
            if (ph > 0) OnBossPhase(e, ph);
        }
    }
    if (spawnQueue_.empty()) return;

    for (const EnemySpawnRequest& req : spawnQueue_) {
        if (req.roomId < 0) continue;
        int aliveInRoom = 0;
        for (const Enemy& e : enemies_) {
            if (e.alive && e.roomId == req.roomId && e.type != EnemyType::Boss) ++aliveInRoom;
        }
        if (aliveInRoom >= 8) continue;
        const Room& room = world_.GetRoom(req.roomId);
        const Vector2 p = world_.RandomSpawnPoint(room, player_.pos, 160.0f, 22.0f);
        enemies_.push_back(Enemy::Make(req.type, p, req.roomId, 1.0f + 0.1f * static_cast<float>(room.depth)));
        fx_.SpawnPortal(p, enemies_.back().BaseColor());
    }
    spawnQueue_.clear();
}

void Game::ResolveEnemyCollisions() {
    const std::size_t n = enemies_.size();
    // Враги расталкивают друг друга (босса - нет).
    for (std::size_t i = 0; i < n; ++i) {
        Enemy& a = enemies_[i];
        if (!a.alive || a.IsSpawning()) continue;
        for (std::size_t j = i + 1; j < n; ++j) {
            Enemy& b = enemies_[j];
            if (!b.alive || b.IsSpawning()) continue;
            const Vector2 d = b.pos - a.pos;
            const float dist = mu::Len(d);
            const float minD = a.radius + b.radius;
            if (dist >= minD) continue;
            const Vector2 nrm = dist > 0.001f ? d / dist : Vector2{ 1.0f, 0.0f };
            const float overlap = minD - dist;
            const float wa = a.type == EnemyType::Boss ? 0.0f : 1.0f;
            const float wb = b.type == EnemyType::Boss ? 0.0f : 1.0f;
            const float total = wa + wb;
            if (total <= 0.0f) continue;
            world_.MoveCircle(a.pos, nrm * (-overlap * wa / total), a.radius);
            world_.MoveCircle(b.pos, nrm * (overlap * wb / total), b.radius);
        }
    }

    // Контактный урон от врагов.
    if (!player_.alive) return;
    for (Enemy& e : enemies_) {
        if (!e.alive || e.IsSpawning()) continue;
        const Vector2 d = player_.pos - e.pos;
        const float dist = mu::Len(d);
        const float minD = e.radius + player_.radius;
        if (dist >= minD) continue;
        const Vector2 nrm = dist > 0.001f ? d / dist : Vector2{ 1.0f, 0.0f };
        if (DamagePlayer(e.ContactDamage(), e.pos)) {
            if (e.IsLunging()) {
                e.chaserState = ChaserState::Recover;
                e.stateTimer = 0.6f;
                e.attackCooldown = 1.5f;
            }
            e.vel = -nrm * 220.0f;
        }
        const float overlap = minD - dist;
        if (e.type == EnemyType::Boss) {
            world_.MoveCircle(player_.pos, nrm * overlap, player_.radius);
        } else {
            world_.MoveCircle(e.pos, -nrm * (overlap * 0.6f), e.radius);
            world_.MoveCircle(player_.pos, nrm * (overlap * 0.4f), player_.radius);
        }
    }
}

bool Game::DamagePlayer(float dmg, Vector2 source) {
    if (!player_.TakeDamage(dmg, source, fx_)) return false;
    hurtFlash_ = 1.0f;
    return true;
}

void Game::UpdateBullets(float h) {
    for (std::size_t i = 0; i < bullets_.size(); ++i) {
        Projectile& b = bullets_[i];
        if (!b.alive) continue;
        b.pos += b.vel * h;
        b.life -= h;
        if (b.life <= 0.0f) {
            b.alive = false;
            continue;
        }
        if (world_.PointSolid(b.pos)) {  // попадание в стену
            b.alive = false;
            fx_.Hit(b.pos, -mu::Norm(b.vel), b.color);
            continue;
        }
        if (b.fromPlayer) {
            for (Enemy& e : enemies_) {
                if (!e.alive || e.IsSpawning()) continue;
                const float rr = e.radius + b.radius;
                if (mu::DistSq(b.pos, e.pos) > rr * rr) continue;
                b.alive = false;
                fx_.Hit(b.pos, -mu::Norm(b.vel), e.BaseColor());
                if (e.TakeDamage(b.damage)) OnEnemyKilled(e);
                break;
            }
        } else if (player_.alive && !player_.IsDashing()) {
            const float rr = player_.radius + b.radius;
            if (mu::DistSq(b.pos, player_.pos) <= rr * rr) {
                b.alive = false;
                DamagePlayer(b.damage, b.pos);
            }
        }
    }
}

void Game::UpdatePickups(float h) {
    for (Pickup& p : pickups_) {
        if (!p.alive) continue;
        if (!p.Update(h, world_, player_)) continue;
        p.alive = false;
        switch (p.type) {
            case PickupType::Health:
                player_.Heal(static_cast<float>(p.amount));
                fx_.Sparkle(player_.pos, pal::kHealth);
                break;
            case PickupType::Ammo:
                player_.AddAmmo(p.amount);
                fx_.Sparkle(player_.pos, pal::kAmmo);
                break;
            case PickupType::Essence:
                player_.AddEssence(p.amount);
                score_ += 10 * p.amount;
                fx_.Sparkle(player_.pos, pal::kEssence);
                break;
        }
    }
}

void Game::SpawnPickup(PickupType type, Vector2 pos, int amount) {
    pickups_.push_back(Pickup::Make(type, pos, amount));
}

void Game::OnEnemyKilled(Enemy& e) {
    ++kills_;
    score_ += e.ScoreValue();
    float scale = 0.8f;
    float trauma = 0.1f;
    switch (e.type) {
        case EnemyType::Chaser: scale = 0.8f; trauma = 0.08f; break;
        case EnemyType::Shooter: scale = 0.9f; trauma = 0.1f; break;
        case EnemyType::Tank: scale = 1.4f; trauma = 0.25f; break;
        case EnemyType::Boss: scale = 2.4f; trauma = 1.0f; break;
    }
    fx_.Explosion(e.pos, scale, e.BaseColor());
    fx_.shake.AddTrauma(trauma);

    // Выпадение бонусов: эссенция всегда, аптечки и патроны - по шансу.
    for (int i = 0; i < e.EssenceDrop(); ++i) SpawnPickup(PickupType::Essence, e.pos, 1);
    float hpChance = e.type == EnemyType::Chaser ? 0.07f : (e.type == EnemyType::Shooter ? 0.10f : 0.35f);
    if (player_.hp < player_.maxHp * 0.4f) hpChance += 0.15f;
    float ammoChance = e.type == EnemyType::Chaser ? 0.12f : 0.20f;
    if (player_.TotalAmmo() < 40) ammoChance += 0.35f;
    if (e.type != EnemyType::Boss) {
        if (rng::Chance(hpChance)) SpawnPickup(PickupType::Health, e.pos, 25);
        if (rng::Chance(ammoChance)) SpawnPickup(PickupType::Ammo, e.pos, 24);
    }
    if (e.type == EnemyType::Boss) OnBossKilled(e);
}

void Game::OnBossPhase(const Enemy& boss, int phase) {
    const Room& room = world_.GetRoom(boss.roomId);
    SpawnPickup(PickupType::Health, world_.RandomSpawnPoint(room, boss.pos, 120.0f, 10.0f), 30);
    SpawnPickup(PickupType::Ammo, world_.RandomSpawnPoint(room, boss.pos, 120.0f, 10.0f), 30);
    SpawnPickup(PickupType::Ammo, world_.RandomSpawnPoint(room, boss.pos, 120.0f, 10.0f), 30);
    ShowBanner(TextFormat("CORE PHASE %d - SYSTEMS OVERLOADED", phase), Color{ 255, 120, 90, 255 }, 2.5f);
}

void Game::OnBossKilled(const Enemy& boss) {
    runEnd_ = RunEnd::Won;
    endTimer_ = 3.2f;
    timeScale_ = 0.4f;
    bossBlastTimer_ = 0.0f;
    bossDeathPos_ = boss.pos;
    for (Enemy& e : enemies_) {
        if (!e.alive || e.roomId != boss.roomId) continue;
        e.alive = false;
        fx_.Explosion(e.pos, 1.0f, e.BaseColor());
    }
    for (Projectile& b : bullets_) {
        if (b.fromPlayer || !b.alive) continue;
        b.alive = false;
        fx_.Sparkle(b.pos, b.color);
    }
    fx_.shake.AddTrauma(1.0f);
    ShowBanner("MAIN CORE DESTROYED", pal::kEssence, 3.5f);
}

void Game::UpdateCamera(float dt) {
    const Vector2 c = ScreenCenter();
    Vector2 look{ 0.0f, 0.0f };
    if (state_ == GameState::Playing && player_.alive) look = (GetMousePosition() - c) * 0.12f;  // небольшой "заглядывание" к курсору
    const Vector2 desired = player_.pos + look;
    camPos_ += (desired - camPos_) * (1.0f - std::exp(-9.0f * dt));

    const Vector2 sh = fx_.shake.Offset();
    camera_.target = Vector2{ std::round(camPos_.x), std::round(camPos_.y) };
    camera_.offset = Vector2{ std::round(c.x + sh.x), std::round(c.y + sh.y) };
    camera_.rotation = 0.0f;
    camera_.zoom = 1.0f;
}

// ============================================================================
//  Отрисовка
// ============================================================================
void Game::DrawPlaying(bool hud, bool crosshair) {
    BeginMode2D(camera_);
    world_.Draw(camera_, time_);
    for (const Pickup& p : pickups_) p.Draw();
    for (const Enemy& e : enemies_) e.Draw(time_);
    for (const Enemy& e : enemies_) e.DrawHealthBar();
    for (const Projectile& b : bullets_) b.Draw();
    player_.Draw(time_);
    fx_.Draw();
    EndMode2D();

    DrawVignette();
    if (!hud) return;

    ui::HudState state;
    state.player = &player_;
    state.world = &world_;
    state.score = score_;
    state.runTime = runTime_;
    state.time = time_;
    for (const Enemy& e : enemies_) {
        if (e.alive && e.type == EnemyType::Boss) state.boss = &e;
    }
    if (activeRoom_ >= 0) {
        const Room& r = world_.GetRoom(activeRoom_);
        state.roomActive = true;
        state.wave = r.wavesSpawned;
        state.waveTotal = r.wavesTotal;
        for (const Enemy& e : enemies_) {
            if (e.alive && e.roomId == r.id) ++state.hostiles;
        }
    }
    ui::DrawHUD(state);
    DrawBanner();
    if (crosshair) ui::DrawCrosshair(GetMousePosition(), player_.IsReloading(), time_);
}

void Game::DrawVignette() const {
    float a = hurtFlash_ * 0.55f;
    if (player_.alive && player_.maxHp > 0.0f) {
        const float ratio = player_.hp / player_.maxHp;
        if (ratio < 0.3f) {  // предупреждение о низком здоровье
            a = std::max(a, (0.3f - ratio) / 0.3f * 0.35f * (0.6f + 0.4f * std::sin(time_ * 7.0f)));
        }
    }
    if (a <= 0.01f) return;
    const unsigned char alpha = static_cast<unsigned char>(255.0f * mu::Clamp01(a));
    const Color c{ 255, 30, 50, alpha };
    const Color clear{ 255, 30, 50, 0 };
    const int W = cfg::kScreenW;
    const int H = cfg::kScreenH;
    DrawRectangleGradientV(0, 0, W, 150, c, clear);
    DrawRectangleGradientV(0, H - 150, W, 150, clear, c);
    DrawRectangleGradientH(0, 0, 180, H, c, clear);
    DrawRectangleGradientH(W - 180, 0, 180, H, clear, c);
}

void Game::DrawBanner() const {
    if (bannerTimer_ <= 0.0f) return;
    const float alpha = mu::Clamp01(std::min(bannerTimer_ / 0.5f, (bannerTotal_ - bannerTimer_) / 0.15f));
    Color c = bannerColor_;
    c.a = static_cast<unsigned char>(255.0f * alpha);
    ui::LabelCentered(bannerText_.c_str(), cfg::kScreenW / 2, 190, 34, c);
}

void Game::DrawStars() const {
    for (const Star& s : stars_) DrawCircleV(s.pos, s.size, Fade(WHITE, 0.15f + s.size * 0.2f));
}

void Game::DrawMenu() {
    DrawStars();
    const int W = cfg::kScreenW;
    const int H = cfg::kScreenH;

    // Заголовок с "глитч"-эффектом
    const char* title = "VOID PROTOCOL";
    const int size = 92;
    const int tx = (W - MeasureText(title, size)) / 2;
    const int ty = 120;
    const int off = static_cast<int>(3.0f + 2.0f * std::sin(time_ * 3.0f));
    DrawText(title, tx - off, ty, size, Color{ 255, 60, 200, 140 });
    DrawText(title, tx + off, ty, size, Color{ 80, 235, 255, 140 });
    DrawText(title, tx, ty, size, WHITE);
    ui::LabelCentered("2D SCI-FI ACTION ROGUELIKE", W / 2, ty + 104, 22, pal::kUi);

    menuButtons_.Draw(time_);
    if (bestScore_ > 0) ui::LabelCentered(TextFormat("BEST SCORE  %d", bestScore_), W / 2, 530, 22, pal::kAmmo);

    ui::LabelCentered("WASD - MOVE    MOUSE - AIM    LMB - FIRE    R - RELOAD    SPACE / SHIFT - DASH    ESC / P - PAUSE",
                      W / 2, H - 76, 18, LIGHTGRAY);
    ui::LabelCentered("Purge the station, collect essence, survive the waves and destroy the MAIN CORE.", W / 2, H - 48, 18, GRAY);
}

void Game::DrawPauseOverlay() {
    DrawRectangle(0, 0, cfg::kScreenW, cfg::kScreenH, Fade(BLACK, 0.6f));
    ui::LabelCentered("PAUSED", cfg::kScreenW / 2, 140, 72, WHITE);
    pauseButtons_.Draw(time_);
    ui::LabelCentered("ESC / P - resume", cfg::kScreenW / 2, cfg::kScreenH - 60, 18, GRAY);
}

void Game::DrawUpgradeOverlay() {
    DrawRectangle(0, 0, cfg::kScreenW, cfg::kScreenH, Fade(BLACK, 0.68f));
    ui::LabelCentered("LEVEL UP", cfg::kScreenW / 2, 90, 64, pal::kEssence);
    ui::LabelCentered(TextFormat("LEVEL %d  -  CHOOSE AN UPGRADE", player_.level), cfg::kScreenW / 2, 164, 22, WHITE);
    for (int i = 0; i < 3; ++i) {
        ui::DrawUpgradeCard(ui::UpgradeCardRect(i), choices_[static_cast<std::size_t>(i)], i, i == upgradeSelected_, time_);
    }
    ui::LabelCentered("Press 1 / 2 / 3, use arrows + ENTER, or click a card", cfg::kScreenW / 2, 540, 18, GRAY);
}

void Game::DrawEndScreen(bool victory) {
    const int W = cfg::kScreenW;
    DrawRectangle(0, 0, W, cfg::kScreenH, Fade(BLACK, 0.72f));
    DrawStars();
    ui::LabelCentered(victory ? "STATION PURGED" : "SYSTEM FAILURE", W / 2, 80, 72,
                      victory ? pal::kEssence : Color{ 255, 70, 90, 255 });

    int y = 190;
    const int step = 36;
    ui::LabelCentered(TextFormat("SCORE  %d", finalScore_), W / 2, y, 34, WHITE);
    y += step + 8;
    if (newBest_) ui::LabelCentered("NEW RECORD!", W / 2, y, 24, pal::kAmmo);
    else ui::LabelCentered(TextFormat("BEST  %d", bestScore_), W / 2, y, 24, pal::kAmmo);
    y += step;
    ui::LabelCentered(TextFormat("HOSTILES DESTROYED  %d", kills_), W / 2, y, 24, LIGHTGRAY);
    y += step;
    ui::LabelCentered(TextFormat("ROOMS CLEARED  %d", roomsCleared_), W / 2, y, 24, LIGHTGRAY);
    y += step;
    ui::LabelCentered(TextFormat("LEVEL  %d", player_.level), W / 2, y, 24, LIGHTGRAY);
    y += step;
    const int total = static_cast<int>(runTime_);
    ui::LabelCentered(TextFormat("TIME  %02d:%02d", total / 60, total % 60), W / 2, y, 24, LIGHTGRAY);

    (victory ? victoryButtons_ : overButtons_).Draw(time_);
}
