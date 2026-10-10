#pragma once
// ============================================================================
//  Void Protocol - общие константы, векторная математика, ГСЧ и палитра.
//  raymath.h намеренно НЕ подключается: операторы для Vector2 определены тут,
//  иначе они конфликтовали бы с операторами из raymath.
// ============================================================================
#include "raylib.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <random>

namespace cfg {
inline constexpr int kScreenW = 1280;
inline constexpr int kScreenH = 720;
inline constexpr int kTile = 32;           // размер тайла в пикселях
inline constexpr float kTileF = 32.0f;
inline constexpr int kCellsX = 5;          // сетка ячеек для генерации комнат
inline constexpr int kCellsY = 4;
inline constexpr int kCellW = 28;          // размер ячейки в тайлах
inline constexpr int kCellH = 24;
inline constexpr int kMapW = kCellsX * kCellW;  // 140 тайлов
inline constexpr int kMapH = kCellsY * kCellH;  // 96 тайлов
inline constexpr float kSimStep = 1.0f / 120.0f;  // максимальный шаг симуляции
}  // namespace cfg

// ---------------------------------------------------------------- Vector2 ops
inline Vector2 operator+(Vector2 a, Vector2 b) { return Vector2{ a.x + b.x, a.y + b.y }; }
inline Vector2 operator-(Vector2 a, Vector2 b) { return Vector2{ a.x - b.x, a.y - b.y }; }
inline Vector2 operator-(Vector2 a) { return Vector2{ -a.x, -a.y }; }
inline Vector2 operator*(Vector2 a, float s) { return Vector2{ a.x * s, a.y * s }; }
inline Vector2 operator*(float s, Vector2 a) { return Vector2{ a.x * s, a.y * s }; }
inline Vector2 operator/(Vector2 a, float s) { return Vector2{ a.x / s, a.y / s }; }
inline Vector2& operator+=(Vector2& a, Vector2 b) { a.x += b.x; a.y += b.y; return a; }
inline Vector2& operator-=(Vector2& a, Vector2 b) { a.x -= b.x; a.y -= b.y; return a; }
inline Vector2& operator*=(Vector2& a, float s) { a.x *= s; a.y *= s; return a; }

struct IVec2 {
    int x = 0;
    int y = 0;
};

namespace mu {
inline float LenSq(Vector2 v) { return v.x * v.x + v.y * v.y; }
inline float Len(Vector2 v) { return std::sqrt(LenSq(v)); }
inline float Dist(Vector2 a, Vector2 b) { return Len(b - a); }
inline float DistSq(Vector2 a, Vector2 b) { return LenSq(b - a); }
inline Vector2 Norm(Vector2 v) {
    const float l = Len(v);
    return l > 1e-5f ? Vector2{ v.x / l, v.y / l } : Vector2{ 0.0f, 0.0f };
}
inline float Dot(Vector2 a, Vector2 b) { return a.x * b.x + a.y * b.y; }
inline Vector2 FromAngle(float a) { return Vector2{ std::cos(a), std::sin(a) }; }
inline float AngleOf(Vector2 v) { return std::atan2(v.y, v.x); }
inline Vector2 Perp(Vector2 v) { return Vector2{ -v.y, v.x }; }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float Clamp01(float t) { return t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t); }
inline Color LerpColor(Color a, Color b, float t) {
    t = Clamp01(t);
    auto mix = [t](unsigned char x, unsigned char y) {
        return static_cast<unsigned char>(static_cast<float>(x) + (static_cast<float>(y) - static_cast<float>(x)) * t);
    };
    return Color{ mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b), mix(a.a, b.a) };
}
}  // namespace mu

namespace rng {
inline std::mt19937& Engine() {
    static std::mt19937 engine(static_cast<std::uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    return engine;
}
inline float Range(float a, float b) {
    std::uniform_real_distribution<float> d(a, b);
    return d(Engine());
}
inline int Int(int a, int b) {  // включительно
    std::uniform_int_distribution<int> d(a, b);
    return d(Engine());
}
inline bool Chance(float p) { return Range(0.0f, 1.0f) < p; }
inline Vector2 Direction() { return mu::FromAngle(Range(0.0f, 2.0f * PI)); }
}  // namespace rng

namespace pal {
inline constexpr Color kBackground{ 7, 9, 16, 255 };
inline constexpr Color kPlayer{ 90, 235, 255, 255 };
inline constexpr Color kPlayerDark{ 18, 60, 80, 255 };
inline constexpr Color kChaser{ 255, 80, 90, 255 };
inline constexpr Color kShooter{ 255, 170, 60, 255 };
inline constexpr Color kTank{ 175, 105, 255, 255 };
inline constexpr Color kBoss{ 255, 60, 200, 255 };
inline constexpr Color kEssence{ 80, 255, 220, 255 };
inline constexpr Color kHealth{ 90, 255, 130, 255 };
inline constexpr Color kAmmo{ 255, 220, 90, 255 };
inline constexpr Color kUi{ 120, 190, 255, 255 };
}  // namespace pal
