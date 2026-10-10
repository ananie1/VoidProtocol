#pragma once
// Главный класс игры: игровой цикл, стейт-машина, симуляция и отрисовка.
#include "Common.h"
#include "Effects.h"
#include "Enemy.h"
#include "Pickup.h"
#include "Player.h"
#include "Projectile.h"
#include "UI.h"
#include "World.h"

#include <array>
#include <string>
#include <vector>

enum class GameState { Menu, Playing, Paused, Upgrade, GameOver, Victory };
enum class RunEnd { None, Dead, Won };

class Game {
public:
    Game();
    ~Game();
    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void Run();

private:
    struct Star {
        Vector2 pos;
        float speed;
        float size;
    };

    // --- Игровой цикл и состояния
    void Update(float dt);
    void Draw();
    void SetState(GameState s);
    void StartRun();
    void FinishRun(bool victory);

    // --- Меню и экраны
    void UpdateMenu(float dt);
    void UpdatePaused();
    void UpdateUpgrade();
    void UpdateEndScreen();
    void DrawMenu();
    void DrawPauseOverlay();
    void DrawUpgradeOverlay();
    void DrawEndScreen(bool victory);
    void DrawStars() const;
    void UpdateStars(float dt);

    // --- Игровой процесс
    PlayerInput PollInput() const;
    void UpdatePlaying(float dt);
    void SimStep(float h, const PlayerInput& in);
    void UpdateRoomLogic(float h);
    void EnterRoom(Room& room);
    void SpawnWave(Room& room);
    void ClearRoom(Room& room);
    void UpdateEnemies(float h);
    void ResolveEnemyCollisions();
    void UpdateBullets(float h);
    void UpdatePickups(float h);
    void OnEnemyKilled(Enemy& e);
    void OnBossPhase(const Enemy& boss, int phase);
    void OnBossKilled(const Enemy& boss);
    void SpawnPickup(PickupType type, Vector2 pos, int amount);
    bool DamagePlayer(float dmg, Vector2 source);
    void RollUpgrades();
    EnemyType PickEnemyType(int depth) const;
    void UpdateCamera(float dt);

    // --- Отрисовка игрового процесса
    void DrawPlaying(bool hud, bool crosshair);
    void DrawVignette() const;
    void DrawBanner() const;
    void ShowBanner(const std::string& text, Color color, float duration);

    // --- Сохранение рекорда
    void LoadBest();
    void SaveBest() const;

    // Состояние
    GameState state_ = GameState::Menu;
    bool quit_ = false;
    float time_ = 0.0f;  // реальное время (для анимаций)

    // Мир и сущности
    World world_;
    Player player_;
    std::vector<Enemy> enemies_;
    std::vector<Projectile> bullets_;
    std::vector<Pickup> pickups_;
    std::vector<EnemySpawnRequest> spawnQueue_;
    Effects fx_;
    Camera2D camera_{};
    Vector2 camPos_{ 0.0f, 0.0f };

    // Прогресс забега
    int currentRoom_ = 0;
    int activeRoom_ = -1;
    float waveDelay_ = 0.0f;
    float flowTimer_ = 0.0f;
    float runTime_ = 0.0f;
    float timeScale_ = 1.0f;
    float fireLock_ = 0.0f;
    float hurtFlash_ = 0.0f;
    int score_ = 0;
    int kills_ = 0;
    int roomsCleared_ = 0;
    int bestScore_ = 0;
    int finalScore_ = 0;
    bool newBest_ = false;
    RunEnd runEnd_ = RunEnd::None;
    float endTimer_ = 0.0f;
    float bossBlastTimer_ = 0.0f;
    Vector2 bossDeathPos_{ 0.0f, 0.0f };

    // Баннер
    std::string bannerText_;
    Color bannerColor_ = WHITE;
    float bannerTimer_ = 0.0f;
    float bannerTotal_ = 1.0f;

    // Выбор улучшений
    std::array<Upgrade, 3> choices_{ Upgrade::Damage, Upgrade::FireRate, Upgrade::MaxHealth };
    int upgradeSelected_ = 0;

    // Меню
    ui::ButtonList menuButtons_;
    ui::ButtonList pauseButtons_;
    ui::ButtonList overButtons_;
    ui::ButtonList victoryButtons_;
    std::vector<Star> stars_;
};
