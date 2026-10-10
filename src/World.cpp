#include "World.h"

#include <cstdlib>

namespace {
constexpr int kDx4[4] = { 1, -1, 0, 0 };
constexpr int kDy4[4] = { 0, 0, 1, -1 };
}  // namespace

// ============================================================================
//  Доступ к тайлам и коллизии
// ============================================================================
Tile World::At(int tx, int ty) const {
    if (!InBounds(tx, ty)) return Tile::Void;
    return tiles_[Index(tx, ty)];
}

bool World::IsSolidTile(int tx, int ty) const { return At(tx, ty) != Tile::Floor; }

bool World::PointSolid(Vector2 p) const {
    return IsSolidTile(static_cast<int>(std::floor(p.x / cfg::kTileF)),
                       static_cast<int>(std::floor(p.y / cfg::kTileF)));
}

bool World::CircleHitsSolid(Vector2 c, float r) const {
    const int x0 = static_cast<int>(std::floor((c.x - r) / cfg::kTileF));
    const int x1 = static_cast<int>(std::floor((c.x + r) / cfg::kTileF));
    const int y0 = static_cast<int>(std::floor((c.y - r) / cfg::kTileF));
    const int y1 = static_cast<int>(std::floor((c.y + r) / cfg::kTileF));
    for (int ty = y0; ty <= y1; ++ty) {
        for (int tx = x0; tx <= x1; ++tx) {
            if (!IsSolidTile(tx, ty)) continue;
            // Круг против AABB тайла: ближайшая точка прямоугольника к центру круга.
            const float rx = static_cast<float>(tx) * cfg::kTileF;
            const float ry = static_cast<float>(ty) * cfg::kTileF;
            const float nx = std::clamp(c.x, rx, rx + cfg::kTileF);
            const float ny = std::clamp(c.y, ry, ry + cfg::kTileF);
            const float dx = c.x - nx;
            const float dy = c.y - ny;
            if (dx * dx + dy * dy < r * r) return true;
        }
    }
    return false;
}

bool World::MoveCircle(Vector2& pos, Vector2 delta, float r) const {
    bool hit = false;
    const float maxComp = std::max(std::fabs(delta.x), std::fabs(delta.y));
    const int steps = std::max(1, static_cast<int>(std::ceil(maxComp / 8.0f)));
    const Vector2 step = delta / static_cast<float>(steps);
    for (int i = 0; i < steps; ++i) {
        if (step.x != 0.0f) {
            const Vector2 t{ pos.x + step.x, pos.y };
            if (!CircleHitsSolid(t, r)) pos = t; else hit = true;
        }
        if (step.y != 0.0f) {
            const Vector2 t{ pos.x, pos.y + step.y };
            if (!CircleHitsSolid(t, r)) pos = t; else hit = true;
        }
    }
    return hit;
}

bool World::HasLineOfSight(Vector2 a, Vector2 b) const {
    const Vector2 d = b - a;
    const float dist = mu::Len(d);
    const int steps = std::max(1, static_cast<int>(dist / 10.0f));
    for (int i = 1; i < steps; ++i) {
        const Vector2 p = a + d * (static_cast<float>(i) / static_cast<float>(steps));
        if (PointSolid(p)) return false;
    }
    return true;
}

int World::RoomAt(Vector2 p) const {
    const int tx = static_cast<int>(std::floor(p.x / cfg::kTileF));
    const int ty = static_cast<int>(std::floor(p.y / cfg::kTileF));
    if (!InBounds(tx, ty)) return -1;
    return roomMap_[Index(tx, ty)];
}

int World::RoomAtInner(Vector2 p) const {
    const int id = RoomAt(p);
    if (id < 0) return -1;
    const Room& r = GetRoom(id);
    const int tx = static_cast<int>(std::floor(p.x / cfg::kTileF));
    const int ty = static_cast<int>(std::floor(p.y / cfg::kTileF));
    if (tx < r.x + 1 || tx > r.x + r.w - 2 || ty < r.y + 1 || ty > r.y + r.h - 2) return -1;
    return id;
}

void World::LockRoom(int id) {
    for (const IVec2& t : GetRoom(id).doorTiles) tiles_[Index(t.x, t.y)] = Tile::Door;
}

void World::UnlockRoom(int id) {
    for (const IVec2& t : GetRoom(id).doorTiles) tiles_[Index(t.x, t.y)] = Tile::Floor;
}

// ============================================================================
//  Поле расстояний (волновой алгоритм) для поиска пути врагов
// ============================================================================
void World::UpdateFlowField(Vector2 target) {
    std::fill(flow_.begin(), flow_.end(), -1);
    const int tx = static_cast<int>(std::floor(target.x / cfg::kTileF));
    const int ty = static_cast<int>(std::floor(target.y / cfg::kTileF));
    if (!InBounds(tx, ty) || IsSolidTile(tx, ty)) return;

    bfsQueue_.clear();
    flow_[Index(tx, ty)] = 0;
    bfsQueue_.push_back(ty * cfg::kMapW + tx);
    for (std::size_t head = 0; head < bfsQueue_.size(); ++head) {
        const int cur = bfsQueue_[head];
        const int cx = cur % cfg::kMapW;
        const int cy = cur / cfg::kMapW;
        const int d = flow_[static_cast<std::size_t>(cur)];
        for (int k = 0; k < 4; ++k) {
            const int nx = cx + kDx4[k];
            const int ny = cy + kDy4[k];
            if (!InBounds(nx, ny) || IsSolidTile(nx, ny)) continue;
            const std::size_t idx = Index(nx, ny);
            if (flow_[idx] >= 0) continue;
            flow_[idx] = d + 1;
            bfsQueue_.push_back(ny * cfg::kMapW + nx);
        }
    }
}

Vector2 World::FlowDirection(Vector2 from) const {
    const int tx = static_cast<int>(std::floor(from.x / cfg::kTileF));
    const int ty = static_cast<int>(std::floor(from.y / cfg::kTileF));
    if (!InBounds(tx, ty)) return Vector2{ 0.0f, 0.0f };
    int best = flow_[Index(tx, ty)];
    if (best < 0) return Vector2{ 0.0f, 0.0f };
    bool found = false;
    IVec2 bestTile{ tx, ty };
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) continue;
            const int nx = tx + dx;
            const int ny = ty + dy;
            if (!InBounds(nx, ny) || IsSolidTile(nx, ny)) continue;
            // Не срезаем углы стен по диагонали.
            if (dx != 0 && dy != 0 && (IsSolidTile(tx + dx, ty) || IsSolidTile(tx, ty + dy))) continue;
            const int v = flow_[Index(nx, ny)];
            if (v >= 0 && v < best) {
                best = v;
                bestTile = IVec2{ nx, ny };
                found = true;
            }
        }
    }
    if (!found) return Vector2{ 0.0f, 0.0f };
    const Vector2 center{ (static_cast<float>(bestTile.x) + 0.5f) * cfg::kTileF,
                          (static_cast<float>(bestTile.y) + 0.5f) * cfg::kTileF };
    return mu::Norm(center - from);
}

Vector2 World::RandomSpawnPoint(const Room& room, Vector2 avoid, float minDist, float radius) const {
    for (int i = 0; i < 120; ++i) {
        if (i == 60) minDist *= 0.5f;  // если место не находится - ослабляем требование
        const int tx = rng::Int(room.x + 1, room.x + room.w - 2);
        const int ty = rng::Int(room.y + 1, room.y + room.h - 2);
        const Vector2 p{ (static_cast<float>(tx) + 0.5f) * cfg::kTileF,
                         (static_cast<float>(ty) + 0.5f) * cfg::kTileF };
        if (CircleHitsSolid(p, radius + 6.0f)) continue;
        if (mu::DistSq(p, avoid) < minDist * minDist) continue;
        return p;
    }
    return room.Center();
}

// ============================================================================
//  Процедурная генерация
// ============================================================================
void World::CarveRect(int x, int y, int w, int h) {
    for (int ty = y; ty < y + h; ++ty) {
        for (int tx = x; tx < x + w; ++tx) {
            if (InBounds(tx, ty)) tiles_[Index(tx, ty)] = Tile::Floor;
        }
    }
}

void World::Generate() {
    using namespace cfg;
    tiles_.assign(static_cast<std::size_t>(kMapW) * static_cast<std::size_t>(kMapH), Tile::Void);
    roomMap_.assign(tiles_.size(), -1);
    flow_.assign(tiles_.size(), -1);
    bfsQueue_.clear();
    bfsQueue_.reserve(tiles_.size());
    rooms_.clear();

    auto R = [this](int i) -> Room& { return rooms_[static_cast<std::size_t>(i)]; };
    std::vector<int> cellRoom(static_cast<std::size_t>(kCellsX * kCellsY), -1);
    auto cellAt = [&](int cx, int cy) -> int& { return cellRoom[static_cast<std::size_t>(cy * kCellsX + cx)]; };
    auto cellValid = [](int cx, int cy) { return cx >= 0 && cy >= 0 && cx < kCellsX && cy < kCellsY; };

    auto addRoom = [&](int cx, int cy) {
        Room r;
        r.id = static_cast<int>(rooms_.size());
        r.cellX = cx;
        r.cellY = cy;
        cellAt(cx, cy) = r.id;
        rooms_.push_back(r);
        return r.id;
    };
    auto link = [&](int a, int b) {
        std::vector<int>& na = R(a).neighbors;
        if (std::find(na.begin(), na.end(), b) != na.end()) return;
        na.push_back(b);
        R(b).neighbors.push_back(a);
    };

    // 1) Растим дерево комнат от стартовой ячейки (левый край карты).
    startRoom_ = addRoom(0, rng::Int(0, kCellsY - 1));
    const int target = rng::Int(10, 13);
    for (int guard = 0; static_cast<int>(rooms_.size()) < target && guard < 5000; ++guard) {
        const int from = rng::Int(0, static_cast<int>(rooms_.size()) - 1);
        const int d = rng::Int(0, 3);
        const int nx = R(from).cellX + kDx4[d];
        const int ny = R(from).cellY + kDy4[d];
        if (!cellValid(nx, ny) || cellAt(nx, ny) != -1) continue;
        link(from, addRoom(nx, ny));
    }

    // 2) Дополнительные связи между соседними комнатами создают петли.
    for (int a = 0; a < static_cast<int>(rooms_.size()); ++a) {
        for (int d = 0; d < 4; ++d) {
            const int nx = R(a).cellX + kDx4[d];
            const int ny = R(a).cellY + kDy4[d];
            if (!cellValid(nx, ny)) continue;
            const int b = cellAt(nx, ny);
            if (b > a && rng::Chance(0.3f)) link(a, b);
        }
    }

    // 3) Глубина каждой комнаты (BFS по графу) и выбор комнаты босса - самой дальней.
    std::vector<int> depth(rooms_.size(), -1);
    std::vector<int> queue{ startRoom_ };
    depth[static_cast<std::size_t>(startRoom_)] = 0;
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int cur = queue[head];
        for (int n : R(cur).neighbors) {
            if (depth[static_cast<std::size_t>(n)] >= 0) continue;
            depth[static_cast<std::size_t>(n)] = depth[static_cast<std::size_t>(cur)] + 1;
            queue.push_back(n);
        }
    }
    const int maxDepth = *std::max_element(depth.begin(), depth.end());
    std::vector<int> farthest;
    for (int i = 0; i < static_cast<int>(rooms_.size()); ++i) {
        if (depth[static_cast<std::size_t>(i)] == maxDepth) farthest.push_back(i);
    }
    bossRoom_ = farthest[static_cast<std::size_t>(rng::Int(0, static_cast<int>(farthest.size()) - 1))];

    // 4) Размеры, типы и вырезание комнат (каждая центрирована в своей ячейке).
    for (Room& r : rooms_) {
        r.depth = depth[static_cast<std::size_t>(r.id)];
        if (r.id == startRoom_) {
            r.type = RoomType::Start;
            r.w = 16;
            r.h = 12;
            r.state = RoomState::Cleared;
            r.visited = true;
            r.wavesTotal = 0;
        } else if (r.id == bossRoom_) {
            r.type = RoomType::Boss;
            r.w = 24;
            r.h = 20;
            r.wavesTotal = 1;
        } else {
            r.type = RoomType::Normal;
            r.w = 2 * rng::Int(7, 11);
            r.h = 2 * rng::Int(6, 9);
            r.wavesTotal = std::min(3, 1 + (r.depth + 1) / 2);
        }
        const int ccx = r.cellX * kCellW + kCellW / 2;
        const int ccy = r.cellY * kCellH + kCellH / 2;
        r.x = ccx - r.w / 2;
        r.y = ccy - r.h / 2;
        CarveRect(r.x, r.y, r.w, r.h);
        for (int ty = r.y; ty < r.y + r.h; ++ty) {
            for (int tx = r.x; tx < r.x + r.w; ++tx) roomMap_[Index(tx, ty)] = r.id;
        }
    }

    // 5) Коридоры шириной 3 тайла между центрами связанных комнат.
    for (const Room& a : rooms_) {
        for (int nb : a.neighbors) {
            if (nb < a.id) continue;
            const Room& b = R(nb);
            const int ax = a.cellX * kCellW + kCellW / 2;
            const int ay = a.cellY * kCellH + kCellH / 2;
            const int bx = b.cellX * kCellW + kCellW / 2;
            const int by = b.cellY * kCellH + kCellH / 2;
            if (a.cellY == b.cellY) {
                CarveRect(std::min(ax, bx), ay - 1, std::abs(bx - ax) + 1, 3);
            } else {
                CarveRect(ax - 1, std::min(ay, by), 3, std::abs(by - ay) + 1);
            }
        }
    }

    // 6) Проёмы комнат: тайлы пола на "рамке" вокруг комнаты (их запирают на время боя).
    for (Room& r : rooms_) {
        auto add = [&](int tx, int ty) {
            if (At(tx, ty) == Tile::Floor) r.doorTiles.push_back(IVec2{ tx, ty });
        };
        for (int tx = r.x - 1; tx <= r.x + r.w; ++tx) {
            add(tx, r.y - 1);
            add(tx, r.y + r.h);
        }
        for (int ty = r.y; ty < r.y + r.h; ++ty) {
            add(r.x - 1, ty);
            add(r.x + r.w, ty);
        }
    }

    // 7) Колонны-укрытия. Центральные "дорожки" к проёмам всегда свободны.
    for (Room& r : rooms_) {
        if (r.type == RoomType::Start) continue;
        const int cx = r.x + r.w / 2;
        const int cy = r.y + r.h / 2;
        auto fill = [&](int px, int py, int s) {
            for (int ty = py; ty < py + s; ++ty) {
                for (int tx = px; tx < px + s; ++tx) {
                    if (InBounds(tx, ty)) tiles_[Index(tx, ty)] = Tile::Wall;
                }
            }
        };
        if (r.type == RoomType::Boss) {
            for (int sx = 0; sx < 2; ++sx) {
                for (int sy = 0; sy < 2; ++sy) fill(sx == 0 ? cx - 7 : cx + 6, sy == 0 ? cy - 6 : cy + 5, 2);
            }
            continue;
        }
        for (int qx = 0; qx < 2; ++qx) {
            for (int qy = 0; qy < 2; ++qy) {
                const int count = rng::Int(0, 2);
                for (int k = 0; k < count; ++k) {
                    const int s = rng::Int(1, 2);
                    const int loX = qx == 0 ? r.x + 2 : cx + 3;
                    const int hiX = qx == 0 ? cx - 4 - (s - 1) : r.x + r.w - 3 - (s - 1);
                    const int loY = qy == 0 ? r.y + 2 : cy + 3;
                    const int hiY = qy == 0 ? cy - 4 - (s - 1) : r.y + r.h - 3 - (s - 1);
                    if (hiX < loX || hiY < loY) continue;
                    const int px = rng::Int(loX, hiX);
                    const int py = rng::Int(loY, hiY);
                    // Между колоннами должно оставаться >= 2 свободных тайлов,
                    // иначе тяжёлый дроид (радиус 20) застрянет в узком проходе.
                    bool crowded = false;
                    for (int ty = py - 2; ty <= py + s + 1 && !crowded; ++ty) {
                        for (int tx = px - 2; tx <= px + s + 1; ++tx) {
                            if (At(tx, ty) == Tile::Wall) {
                                crowded = true;
                                break;
                            }
                        }
                    }
                    if (!crowded) fill(px, py, s);
                }
            }
        }
    }

    // 8) Стены: любой пустой тайл рядом (8 соседей) с полом становится стеной.
    for (int ty = 0; ty < kMapH; ++ty) {
        for (int tx = 0; tx < kMapW; ++tx) {
            if (tiles_[Index(tx, ty)] != Tile::Void) continue;
            bool nearFloor = false;
            for (int dy = -1; dy <= 1 && !nearFloor; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (At(tx + dx, ty + dy) == Tile::Floor) {
                        nearFloor = true;
                        break;
                    }
                }
            }
            if (nearFloor) tiles_[Index(tx, ty)] = Tile::Wall;
        }
    }
}

// ============================================================================
//  Отрисовка
// ============================================================================
void World::Draw(const Camera2D& cam, float time) const {
    const Vector2 tl = GetScreenToWorld2D(Vector2{ 0.0f, 0.0f }, cam);
    const Vector2 br = GetScreenToWorld2D(
        Vector2{ static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight()) }, cam);
    const int x0 = std::max(0, static_cast<int>(std::floor(tl.x / cfg::kTileF)) - 1);
    const int y0 = std::max(0, static_cast<int>(std::floor(tl.y / cfg::kTileF)) - 1);
    const int x1 = std::min(cfg::kMapW - 1, static_cast<int>(std::floor(br.x / cfg::kTileF)) + 1);
    const int y1 = std::min(cfg::kMapH - 1, static_cast<int>(std::floor(br.y / cfg::kTileF)) + 1);

    auto open = [this](int tx, int ty) {
        const Tile t = At(tx, ty);
        return t == Tile::Floor || t == Tile::Door;
    };
    const int T = cfg::kTile;
    const Color wallEdge{ 90, 120, 210, 255 };

    for (int ty = y0; ty <= y1; ++ty) {
        for (int tx = x0; tx <= x1; ++tx) {
            const std::size_t idx = Index(tx, ty);
            const Tile t = tiles_[idx];
            if (t == Tile::Void) continue;
            const int px = tx * T;
            const int py = ty * T;

            if (t == Tile::Wall) {
                DrawRectangle(px, py, T, T, Color{ 34, 40, 68, 255 });
                if (open(tx, ty - 1)) DrawRectangle(px, py, T, 4, wallEdge);
                if (open(tx, ty + 1)) DrawRectangle(px, py + T - 4, T, 4, Color{ 60, 80, 150, 255 });
                if (open(tx - 1, ty)) DrawRectangle(px, py, 4, T, Color{ 75, 100, 180, 255 });
                if (open(tx + 1, ty)) DrawRectangle(px + T - 4, py, 4, T, Color{ 75, 100, 180, 255 });
                continue;
            }

            const int rid = roomMap_[idx];
            const bool odd = ((tx + ty) & 1) != 0;
            Color base;
            if (rid >= 0 && rid == bossRoom_) base = odd ? Color{ 40, 22, 40, 255 } : Color{ 46, 26, 46, 255 };
            else if (rid >= 0) base = odd ? Color{ 24, 29, 48, 255 } : Color{ 28, 34, 56, 255 };
            else base = Color{ 18, 22, 38, 255 };
            DrawRectangle(px, py, T, T, base);
            DrawRectangleLines(px, py, T, T, Color{ 255, 255, 255, 8 });

            if (t == Tile::Door) {
                const float pulse = 0.5f + 0.5f * std::sin(time * 9.0f + static_cast<float>(tx + ty));
                DrawRectangle(px, py, T, T, Color{ 255, 40, 70, static_cast<unsigned char>(80.0f + 90.0f * pulse) });
                const int scan = static_cast<int>(std::fmod(time * 60.0f + static_cast<float>(ty * 7), static_cast<float>(T)));
                DrawRectangle(px, py + scan, T, 3, Color{ 255, 180, 190, 200 });
                DrawRectangleLines(px, py, T, T, Color{ 255, 90, 110, 255 });
            }
        }
    }
}

void World::DrawMinimap(Rectangle area, Vector2 playerPos) const {
    DrawRectangleRec(area, Color{ 8, 12, 24, 190 });
    DrawRectangleLinesEx(area, 1.0f, Color{ 90, 130, 220, 160 });

    const float s = std::min((area.width - 10.0f) / static_cast<float>(cfg::kMapW),
                             (area.height - 10.0f) / static_cast<float>(cfg::kMapH));
    const float ox = area.x + (area.width - static_cast<float>(cfg::kMapW) * s) * 0.5f;
    const float oy = area.y + (area.height - static_cast<float>(cfg::kMapH) * s) * 0.5f;

    auto revealed = [this](const Room& r) {
        if (r.visited) return true;
        for (int n : r.neighbors) {
            if (GetRoom(n).visited) return true;
        }
        return false;
    };
    auto centerOf = [&](const Room& r) {
        return Vector2{ ox + (static_cast<float>(r.x) + static_cast<float>(r.w) * 0.5f) * s,
                        oy + (static_cast<float>(r.y) + static_cast<float>(r.h) * 0.5f) * s };
    };

    for (const Room& r : rooms_) {
        if (!revealed(r)) continue;
        for (int n : r.neighbors) {
            if (n < r.id) continue;
            const Room& o = GetRoom(n);
            if (!r.visited && !o.visited) continue;
            DrawLineEx(centerOf(r), centerOf(o), 2.0f, Color{ 90, 120, 190, 200 });
        }
    }

    const int current = RoomAt(playerPos);
    for (const Room& r : rooms_) {
        if (!revealed(r)) continue;
        Color c{ 70, 85, 140, 200 };
        if (r.type == RoomType::Boss) c = Color{ 255, 60, 200, 220 };
        else if (r.state == RoomState::Cleared) c = Color{ 70, 200, 130, 220 };
        else if (r.state == RoomState::Active) c = Color{ 255, 90, 90, 230 };
        const Rectangle rect{ ox + static_cast<float>(r.x) * s, oy + static_cast<float>(r.y) * s,
                              static_cast<float>(r.w) * s, static_cast<float>(r.h) * s };
        DrawRectangleRec(rect, c);
        if (r.id == current) DrawRectangleLinesEx(rect, 2.0f, WHITE);
    }

    const Vector2 pp{ ox + playerPos.x / cfg::kTileF * s, oy + playerPos.y / cfg::kTileF * s };
    DrawCircleV(pp, 2.5f, WHITE);
}
