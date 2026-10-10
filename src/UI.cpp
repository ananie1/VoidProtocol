#include "UI.h"

namespace ui {

// ------------------------------------------------------------------ Текст
void Label(const char* text, int x, int y, int size, Color color) {
    DrawText(text, x + 2, y + 2, size, Fade(BLACK, 0.65f));
    DrawText(text, x, y, size, color);
}

void LabelCentered(const char* text, int cx, int y, int size, Color color) {
    Label(text, cx - MeasureText(text, size) / 2, y, size, color);
}

void LabelRight(const char* text, int rx, int y, int size, Color color) {
    Label(text, rx - MeasureText(text, size), y, size, color);
}

void Bar(Rectangle r, float t, Color fill, Color back, Color border) {
    DrawRectangleRec(r, back);
    DrawRectangleRec(Rectangle{ r.x, r.y, r.width * mu::Clamp01(t), r.height }, fill);
    DrawRectangleLinesEx(r, 1.5f, border);
}

// ------------------------------------------------------------- ButtonList
ButtonList::ButtonList(std::vector<std::string> labels, float topY)
    : labels_(std::move(labels)), topY_(topY) {}

Rectangle ButtonList::RectOf(int index) const {
    const float w = 340.0f;
    const float h = 54.0f;
    const float gap = 16.0f;
    return Rectangle{ (static_cast<float>(cfg::kScreenW) - w) * 0.5f,
                      topY_ + static_cast<float>(index) * (h + gap), w, h };
}

int ButtonList::Update() {
    const int n = static_cast<int>(labels_.size());
    if (n == 0) return -1;
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) selected_ = (selected_ + 1) % n;
    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) selected_ = (selected_ + n - 1) % n;

    const Vector2 mouse = GetMousePosition();
    const Vector2 delta = GetMouseDelta();
    int hovered = -1;
    for (int i = 0; i < n; ++i) {
        if (CheckCollisionPointRec(mouse, RectOf(i))) hovered = i;
    }
    if (hovered >= 0 && (delta.x != 0.0f || delta.y != 0.0f)) selected_ = hovered;

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) return selected_;
    if (hovered >= 0 && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        selected_ = hovered;
        return hovered;
    }
    return -1;
}

void ButtonList::Draw(float time) const {
    for (int i = 0; i < static_cast<int>(labels_.size()); ++i) {
        Rectangle r = RectOf(i);
        const bool sel = (i == selected_);
        if (sel) {
            const float grow = 2.0f + 1.5f * std::sin(time * 6.0f);
            r.x -= grow;
            r.y -= grow;
            r.width += grow * 2.0f;
            r.height += grow * 2.0f;
        }
        DrawRectangleRec(r, sel ? Color{ 28, 58, 108, 240 } : Color{ 12, 16, 32, 215 });
        DrawRectangleLinesEx(r, sel ? 3.0f : 1.5f, sel ? pal::kPlayer : Color{ 80, 100, 160, 200 });
        if (sel) DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), 6, static_cast<int>(r.height), pal::kPlayer);
        LabelCentered(labels_[static_cast<std::size_t>(i)].c_str(), static_cast<int>(r.x + r.width * 0.5f),
                      static_cast<int>(r.y + (r.height - 24.0f) * 0.5f), 24, sel ? WHITE : LIGHTGRAY);
    }
}

// -------------------------------------------------------------------- HUD
void DrawHUD(const HudState& h) {
    const Player& p = *h.player;
    const int W = cfg::kScreenW;
    const int H = cfg::kScreenH;

    // Здоровье
    const float hpT = mu::Clamp01(p.hp / p.maxHp);
    Bar(Rectangle{ 20.0f, 20.0f, 300.0f, 26.0f }, hpT,
        mu::LerpColor(Color{ 235, 55, 80, 255 }, Color{ 70, 230, 130, 255 }, hpT), Color{ 16, 20, 36, 220 },
        Color{ 140, 170, 230, 200 });
    LabelCentered(TextFormat("HP  %d / %d", static_cast<int>(std::ceil(p.hp)), static_cast<int>(p.maxHp)), 170, 25, 18, WHITE);

    // Рывок
    const float dashT = p.dashCooldown <= 0.0f ? 1.0f : 1.0f - p.dashCooldown / p.dashCooldownMax;
    Bar(Rectangle{ 20.0f, 52.0f, 300.0f, 8.0f }, dashT, dashT >= 1.0f ? pal::kPlayer : Color{ 60, 120, 160, 255 },
        Color{ 16, 20, 36, 220 }, Color{ 100, 140, 200, 160 });
    Label("DASH", 328, 49, 14, dashT >= 1.0f ? pal::kPlayer : GRAY);

    // Эссенция / уровень
    Bar(Rectangle{ 20.0f, 68.0f, 300.0f, 14.0f },
        static_cast<float>(p.essence) / static_cast<float>(p.essenceToNext), pal::kEssence, Color{ 16, 20, 36, 220 },
        Color{ 80, 200, 190, 180 });
    Label(TextFormat("LV %d    ESSENCE %d / %d", p.level, p.essence, p.essenceToNext), 20, 88, 16, pal::kEssence);

    // Очки и время
    LabelCentered(TextFormat("SCORE  %d", h.score), W / 2, 16, 26, WHITE);
    const int total = static_cast<int>(h.runTime);
    LabelCentered(TextFormat("%02d:%02d", total / 60, total % 60), W / 2, 46, 18, pal::kUi);

    if (h.roomActive) {
        LabelCentered(TextFormat("WAVE %d / %d     HOSTILES %d", h.wave, h.waveTotal, h.hostiles), W / 2, 72, 20,
                      Color{ 255, 120, 120, 255 });
    }

    // Босс
    if (h.boss != nullptr) {
        const Enemy& b = *h.boss;
        LabelCentered("MAIN CORE", W / 2, 100, 18, pal::kBoss);
        Bar(Rectangle{ static_cast<float>(W) * 0.5f - 300.0f, 122.0f, 600.0f, 16.0f }, b.hp / b.maxHp, pal::kBoss,
            Color{ 30, 10, 28, 230 }, Color{ 255, 120, 220, 220 });
    }

    // Патроны
    const int rx = W - 30;
    const int by = H - 96;
    if (p.IsReloading()) {
        const bool blink = std::fmod(h.time * 6.0f, 2.0f) < 1.0f;
        LabelRight("RELOADING", rx, by, 30, blink ? pal::kAmmo : Color{ 200, 170, 60, 255 });
        Bar(Rectangle{ static_cast<float>(rx) - 220.0f, static_cast<float>(by) + 40.0f, 220.0f, 10.0f }, p.ReloadProgress(),
            pal::kAmmo, Color{ 16, 20, 36, 220 }, Color{ 200, 170, 60, 200 });
    } else {
        const Color magColor = p.mag <= p.magSize / 4 ? Color{ 255, 90, 90, 255 } : WHITE;
        LabelRight(TextFormat("%d", p.mag), rx - 100, by - 8, 64, magColor);
        Label(TextFormat("/ %d", p.reserve), rx - 92, by + 28, 28, pal::kAmmo);
        if (p.mag == 0 && p.reserve == 0) {
            const bool blink = std::fmod(h.time * 4.0f, 2.0f) < 1.0f;
            LabelRight("NO AMMO - CAPACITOR RECHARGING", rx, by - 36, 18, blink ? Color{ 255, 90, 90, 255 } : GRAY);
        } else if (p.mag == 0) {
            LabelRight("PRESS R TO RELOAD", rx, by - 36, 18, pal::kAmmo);
        }
    }

    // Миникарта
    h.world->DrawMinimap(Rectangle{ static_cast<float>(W) - 232.0f, 16.0f, 216.0f, 150.0f }, p.pos);
}

void DrawCrosshair(Vector2 m, bool reloading, float time) {
    const Color c = reloading ? pal::kAmmo : WHITE;
    const float g = 7.0f + (reloading ? 3.0f * std::sin(time * 20.0f) : 0.0f);
    DrawLineEx(Vector2{ m.x - g - 8.0f, m.y }, Vector2{ m.x - g, m.y }, 2.0f, c);
    DrawLineEx(Vector2{ m.x + g, m.y }, Vector2{ m.x + g + 8.0f, m.y }, 2.0f, c);
    DrawLineEx(Vector2{ m.x, m.y - g - 8.0f }, Vector2{ m.x, m.y - g }, 2.0f, c);
    DrawLineEx(Vector2{ m.x, m.y + g }, Vector2{ m.x, m.y + g + 8.0f }, 2.0f, c);
    DrawCircleV(m, 1.5f, c);
}

// ---------------------------------------------------------- Карточки улучшений
Rectangle UpgradeCardRect(int index) {
    const float w = 320.0f;
    const float h = 270.0f;
    const float gap = 40.0f;
    const float total = 3.0f * w + 2.0f * gap;
    return Rectangle{ (static_cast<float>(cfg::kScreenW) - total) * 0.5f + static_cast<float>(index) * (w + gap), 230.0f, w, h };
}

void DrawUpgradeCard(Rectangle r, Upgrade u, int index, bool selected, float time) {
    const Color c = UpgradeColor(u);
    if (selected) {
        r.x -= 6.0f;
        r.y -= 6.0f;
        r.width += 12.0f;
        r.height += 12.0f;
    }
    DrawRectangleRec(r, Color{ 12, 16, 32, 245 });
    DrawRectangleLinesEx(r, selected ? 3.0f : 1.5f, selected ? c : Fade(c, 0.5f));
    DrawRectangle(static_cast<int>(r.x), static_cast<int>(r.y), static_cast<int>(r.width), 8, c);

    const int cx = static_cast<int>(r.x + r.width * 0.5f);
    const Vector2 icon{ static_cast<float>(cx), r.y + 90.0f };
    DrawCircleV(icon, 42.0f, Fade(c, 0.16f));
    DrawPoly(icon, 6, 28.0f, time * 40.0f, c);
    DrawPoly(icon, 6, 18.0f, time * 40.0f, Color{ 12, 16, 32, 255 });
    DrawPoly(icon, 4, 8.0f, -time * 80.0f, c);

    LabelCentered(TextFormat("[%d]", index + 1), cx, static_cast<int>(r.y) + 16, 20, Fade(WHITE, 0.6f));
    LabelCentered(UpgradeName(u), cx, static_cast<int>(r.y) + 150, 28, c);
    LabelCentered(UpgradeDesc(u), cx, static_cast<int>(r.y) + 200, 18, WHITE);
}

}  // namespace ui
