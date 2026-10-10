#pragma once
// Мир: тайловая карта, процедурная генерация комнат, коллизии, поиск пути.
#include "Common.h"

#include <vector>

enum class Tile : std::uint8_t { Void, Wall, Floor, Door };
enum class RoomType { Start, Normal, Boss };
enum class RoomState { Idle, Active, Cleared };

struct Room {
    int id = 0;
    int cellX = 0, cellY = 0;      // ячейка на сетке генерации
    int x = 0, y = 0, w = 0, h = 0;  // внутренняя (проходимая) область в тайлах
    RoomType type = RoomType::Normal;
    RoomState state = RoomState::Idle;
    int depth = 0;                 // расстояние от стартовой комнаты (в комнатах)
    int wavesTotal = 0;
    int wavesSpawned = 0;
    bool visited = false;
    std::vector<int> neighbors;    // id связанных комнат
    std::vector<IVec2> doorTiles;  // тайлы проёмов (блокируются на время боя)

    Vector2 Center() const {
        return Vector2{ (static_cast<float>(x) + static_cast<float>(w) * 0.5f) * cfg::kTileF,
                        (static_cast<float>(y) + static_cast<float>(h) * 0.5f) * cfg::kTileF };
    }
};

class World {
public:
    // Генерирует новый уровень: комнаты на сетке + коридоры + стены + препятствия.
    void Generate();

    Tile At(int tx, int ty) const;
    bool IsSolidTile(int tx, int ty) const;
    bool PointSolid(Vector2 p) const;
    bool CircleHitsSolid(Vector2 c, float r) const;
    // Двигает окружность со скольжением вдоль стен. Возвращает true при столкновении.
    bool MoveCircle(Vector2& pos, Vector2 delta, float r) const;
    bool HasLineOfSight(Vector2 a, Vector2 b) const;

    int RoomAt(Vector2 p) const;       // id комнаты под точкой или -1
    int RoomAtInner(Vector2 p) const;  // то же, но с отступом в 1 тайл от стен
    void LockRoom(int id);
    void UnlockRoom(int id);

    // Волновой алгоритм (BFS) от цели: поле расстояний для обхода препятствий.
    void UpdateFlowField(Vector2 target);
    Vector2 FlowDirection(Vector2 from) const;

    Vector2 RandomSpawnPoint(const Room& room, Vector2 avoid, float minDist, float radius) const;

    void Draw(const Camera2D& cam, float time) const;
    void DrawMinimap(Rectangle area, Vector2 playerPos) const;

    std::vector<Room>& Rooms() { return rooms_; }
    const std::vector<Room>& Rooms() const { return rooms_; }
    Room& GetRoom(int id) { return rooms_[static_cast<std::size_t>(id)]; }
    const Room& GetRoom(int id) const { return rooms_[static_cast<std::size_t>(id)]; }
    int StartRoomId() const { return startRoom_; }
    int BossRoomId() const { return bossRoom_; }
    Vector2 StartPosition() const { return rooms_[static_cast<std::size_t>(startRoom_)].Center(); }

private:
    static std::size_t Index(int tx, int ty) {
        return static_cast<std::size_t>(ty) * static_cast<std::size_t>(cfg::kMapW) + static_cast<std::size_t>(tx);
    }
    static bool InBounds(int tx, int ty) { return tx >= 0 && ty >= 0 && tx < cfg::kMapW && ty < cfg::kMapH; }
    void CarveRect(int x, int y, int w, int h);

    std::vector<Tile> tiles_;
    std::vector<int> roomMap_;  // id комнаты для каждого тайла внутренней области (-1 иначе)
    std::vector<int> flow_;     // расстояния BFS от цели (-1 = недостижимо)
    std::vector<int> bfsQueue_;
    std::vector<Room> rooms_;
    int startRoom_ = 0;
    int bossRoom_ = 0;
};
